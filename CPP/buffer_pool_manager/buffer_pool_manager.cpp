#include "buffer_pool_manager.h"
#include "page_guard.h"

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  replacer_ = std::make_unique<LRUKReplacer>(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManager::~BufferPoolManager() { delete[] pages_; }

auto BufferPoolManager::NewPage(page_id_t *page_id) -> Page * {
  std::scoped_lock scoped_lock(latch_);

  frame_id_t replace_frame;
  if (!free_list_.empty()) {
    replace_frame = free_list_.front();
    free_list_.pop_front();
  } else if (!replacer_->Evict(&replace_frame)) {
    // No evictable frame in both free_list or replacer, just return nullptr
    return nullptr;
  }

  // Now we get the replace_frame id, get the next available page_id
  *page_id = AllocatePage();
  if (pages_[replace_frame].is_dirty_) {
    // First write out the content, using the not-yet-removed page_id_
    disk_manager_->WritePage(pages_[replace_frame].page_id_, pages_[replace_frame].data_);
  }

  // Remove the entry from page_table
  page_table_.erase(pages_[replace_frame].page_id_);
  // 'Pin' the current frame(new page, so to speak)
  replacer_->RecordAccess(replace_frame);
  replacer_->SetEvictable(replace_frame, false);
  // Set metadata
  pages_[replace_frame].ResetMemory();
  pages_[replace_frame].pin_count_ = 1;
  pages_[replace_frame].is_dirty_ = false;
  pages_[replace_frame].page_id_ = *page_id;

  // Register the mapping on page_table
  page_table_[*page_id] = replace_frame;

  return &pages_[replace_frame];
}

auto BufferPoolManager::FetchPage(page_id_t page_id, [[maybe_unused]] AccessType access_type) -> Page * {
  std::scoped_lock scoped_lock(latch_);

  frame_id_t replace_frame;
  if (page_table_.find(page_id) == page_table_.end()) {
    // No existence in the current page_table
    if (free_list_.empty()) {
      // No available page in the current free_list, then try to find it in replacer
      if (!replacer_->Evict(&replace_frame)) {
        // Not available for either free_list or replacer, so just quit
        return nullptr;
      }
    } else {
      // Just grad the frame_id_ from free_list_
      replace_frame = free_list_.front();
      free_list_.pop_front();
    }
  } else {
    // The page requested is currently in the buffer pool, then just return it
    // A new holder of the page
    pages_[page_table_[page_id]].pin_count_ += 1;
    replacer_->RecordAccess(page_table_[page_id]);
    replacer_->SetEvictable(page_table_[page_id], false);
    return &pages_[page_table_[page_id]];
  }

  if (pages_[replace_frame].is_dirty_) {
    disk_manager_->WritePage(pages_[replace_frame].page_id_, pages_[replace_frame].data_);
  }

  page_table_.erase(pages_[replace_frame].page_id_);
  disk_manager_->ReadPage(page_id, pages_[replace_frame].data_);
  // Update metadata for the current page
  pages_[replace_frame].page_id_ = page_id;
  pages_[replace_frame].is_dirty_ = false;
  pages_[replace_frame].pin_count_ = 1;
  // For page_table
  page_table_[page_id] = replace_frame;
  // Also 'Pin' the current page in replacer
  replacer_->RecordAccess(replace_frame);
  replacer_->SetEvictable(replace_frame, false);
  return &pages_[replace_frame];
}

auto BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty, [[maybe_unused]] AccessType access_type) -> bool {
  std::scoped_lock scoped_lock(latch_);
  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }
  if (pages_[page_table_[page_id]].pin_count_ <= 0) {
    return false;
  }

  if (is_dirty) {
    pages_[page_table_[page_id]].is_dirty_ = true;
    // Otherwise, DO NOT change anything
  }
  // Update metadata about the current page
  pages_[page_table_[page_id]].pin_count_ -= 1;
  // If the pin_count_ is 0, set the status to evictable
  if (pages_[page_table_[page_id]].pin_count_ == 0) {
    replacer_->SetEvictable(page_table_[page_id], true);
  }
  return true;
}

auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::scoped_lock scoped_lock(latch_);
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }
  if (page_table_.find(page_id) == page_table_.end()) {
    return false;
  }

  disk_manager_->WritePage(page_id, pages_[page_table_[page_id]].data_);
  pages_[page_table_[page_id]].is_dirty_ = false;

  return true;
}

void BufferPoolManager::FlushAllPages() {
  std::scoped_lock scoped_lock(latch_);
  for (size_t i = 0; i < pool_size_; ++i) {
    FlushPage(static_cast<page_id_t>(i));
  }
}

auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  std::scoped_lock scoped_lock(latch_);

  if (page_id == INVALID_PAGE_ID) {
    return true;
  }
  if (page_table_.find(page_id) == page_table_.end()) {
    return true;
  }
  if (pages_[page_table_[page_id]].pin_count_ > 0) {
    // 'Pinned' page can't be deleted
    return false;
  }
  // Then remove the relevant frame from replacer
  replacer_->Remove(page_table_[page_id]);
  // Add back to the free_list_
  free_list_.emplace_back(page_table_[page_id]);
  // Reset the relevant metadata of the deleted page
  pages_[page_table_[page_id]].ResetMemory();
  pages_[page_table_[page_id]].pin_count_ = 0;
  pages_[page_table_[page_id]].page_id_ = INVALID_PAGE_ID;
  pages_[page_table_[page_id]].is_dirty_ = false;
  // Update page_table
  page_table_.erase(page_id);
  // Imitate freeing the page on the disk
  DeallocatePage(page_id);
  return true;
}

auto BufferPoolManager::AllocatePage() -> page_id_t { return next_page_id_++; }

auto BufferPoolManager::FetchPageBasic(page_id_t page_id) -> BasicPageGuard { return {this, FetchPage(page_id)}; }

auto BufferPoolManager::FetchPageRead(page_id_t page_id) -> ReadPageGuard {
  Page *page = FetchPage(page_id);
  if (page == nullptr) {
    return {this, nullptr};
  }
  page->RLatch();
  return {this, page};
}

auto BufferPoolManager::FetchPageWrite(page_id_t page_id) -> WritePageGuard {
  Page *page = FetchPage(page_id);
  if (page == nullptr) {
    return {this, nullptr};
  }
  page->WLatch();
  return {this, page};
}

auto BufferPoolManager::NewPageGuarded(page_id_t *page_id) -> BasicPageGuard { return {this, NewPage(page_id)}; }
