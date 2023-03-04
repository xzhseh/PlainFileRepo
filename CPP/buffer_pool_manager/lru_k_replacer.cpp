#include "lru_k_replacer.h"

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::scoped_lock scoped_lock(latch_);
  // No available evictable frame
  if (curr_size_ == 0) {
    return false;
  }

  size_t max_difference = 0;
  bool flag = true;
  // Set to the max value of potential size_t type
  size_t least_recent_vis_stamp = SIZE_MAX;
  for (auto &[id, node] : node_store_) {
    if (!node.is_evictable_) {
      continue;
    }
    // The size of history is not k_, which means +inf backward-k distance
    if (node.history_.size() != k_) {
      flag = false;  // Represent there is node with +inf record, no need to record k one then
      if (least_recent_vis_stamp > node.history_.front()) {
        least_recent_vis_stamp = node.history_.front();
        *frame_id = id;
      }
      continue;
    }
    // Else do the normal thing
    if (flag && (current_timestamp_ - node.history_.front()) > max_difference) {
      max_difference = current_timestamp_ - node.history_.front();
      *frame_id = id;
    }
  }
  // Update relevant status
  node_store_[*frame_id].is_evictable_ = false;
  node_store_[*frame_id].history_.clear();
  curr_size_ -= 1;

  return true;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  std::scoped_lock scoped_lock(latch_);

  if (frame_id < 0 || frame_id >= static_cast<frame_id_t>(replacer_size_)) {
    throw bustub::NotImplementedException("Invalid frame_id for RecordAccess");
  }

  if (node_store_.find(frame_id) == node_store_.end()) {
    LRUKNode new_node;
    new_node.history_.push_back(current_timestamp_++);
    node_store_[frame_id] = new_node;
  } else {
    if (node_store_[frame_id].history_.size() == k_) {
      node_store_[frame_id].history_.pop_front();
    }
    node_store_[frame_id].history_.push_back(current_timestamp_++);
  }
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::scoped_lock scoped_lock(latch_);

  if (node_store_.find(frame_id) == node_store_.end()) {
    throw bustub::NotImplementedException("Invalid frame_id for SetEvictable");
  }

  LRUKNode tmp_node = node_store_[frame_id];
  // Needs to increment / decrement the size of replacer
  if (tmp_node.is_evictable_ && !set_evictable) {
    curr_size_ -= 1;
  } else if (!tmp_node.is_evictable_ && set_evictable) {
    curr_size_ += 1;
  } else {
    return;
  }

  node_store_[frame_id].is_evictable_ = set_evictable;
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::scoped_lock scoped_lock(latch_);

  if (node_store_.find(frame_id) == node_store_.end()) {
    return;
  }
  if (!node_store_[frame_id].is_evictable_) {
    //    latch_.unlock();
    //    BUSTUB_ASSERT(false, "Not evictable");
    return;
  }

  node_store_[frame_id].history_.clear();
  node_store_[frame_id].is_evictable_ = false;
  curr_size_ -= 1;
}

auto LRUKReplacer::Size() -> size_t { return curr_size_; }

