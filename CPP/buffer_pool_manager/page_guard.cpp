#include "page_guard.h"
#include "buffer_pool_manager.h"

BasicPageGuard::BasicPageGuard(BasicPageGuard &&that) noexcept {
  this->bpm_ = that.bpm_;
  this->page_ = that.page_;
  this->is_dirty_ = that.is_dirty_;
  // Set to Nullptr
  that.page_ = nullptr;
  that.bpm_ = nullptr;
  that.is_dirty_ = false;
}

void BasicPageGuard::Drop() {
  // We are done with this page
  if (bpm_ != nullptr && page_ != nullptr) {
    // For the destructor, preventing dropping twice...
    bpm_->UnpinPage(PageId(), is_dirty_);
  }
  // Clear the content
  bpm_ = nullptr;
  page_ = nullptr;
  is_dirty_ = false;
}

auto BasicPageGuard::operator=(BasicPageGuard &&that) noexcept -> BasicPageGuard & {
  // Check for self move-assignment
  if (this == &that) {
    return *this;
  }
  // Drop the current page held first
  Drop();
  // Set to the new value
  this->bpm_ = that.bpm_;
  this->page_ = that.page_;
  this->is_dirty_ = that.is_dirty_;
  // Set to Nullptr
  that.page_ = nullptr;
  that.bpm_ = nullptr;
  that.is_dirty_ = false;

  return *this;
}

BasicPageGuard::~BasicPageGuard() { Drop(); }  // NOLINT

ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept {
  guard_.bpm_ = that.guard_.bpm_;
  guard_.page_ = that.guard_.page_;
  guard_.is_dirty_ = that.guard_.is_dirty_;
  // Set to Nullptr
  that.guard_.bpm_ = nullptr;
  that.guard_.page_ = nullptr;
  that.guard_.is_dirty_ = false;
}

auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
  // Check for self move-assignment
  if (this == &that) {
    return *this;
  }
  // Drop the current page held by guard
  Drop();
  // Move the pre-existed move assignment
  guard_.bpm_ = that.guard_.bpm_;
  guard_.page_ = that.guard_.page_;
  guard_.is_dirty_ = that.guard_.is_dirty_;
  // Set to Nullptr
  that.guard_.bpm_ = nullptr;
  that.guard_.page_ = nullptr;
  that.guard_.is_dirty_ = false;

  return *this;
}

void ReadPageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->RUnlatch();
  }
  guard_.is_dirty_ = false;
  guard_.Drop();
}

ReadPageGuard::~ReadPageGuard() { Drop(); }  // NOLINT

WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept {
  guard_.bpm_ = that.guard_.bpm_;
  guard_.page_ = that.guard_.page_;
  guard_.is_dirty_ = that.guard_.is_dirty_;
  // Set to Nullptr
  that.guard_.bpm_ = nullptr;
  that.guard_.page_ = nullptr;
  that.guard_.is_dirty_ = false;
}

auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
  // Check for self move-assignment
  if (this == &that) {
    return *this;
  }
  // Also...
  Drop();
  // Set to the relevant value
  guard_.bpm_ = that.guard_.bpm_;
  guard_.page_ = that.guard_.page_;
  guard_.is_dirty_ = that.guard_.is_dirty_;
  // Set to Nullptr
  that.guard_.bpm_ = nullptr;
  that.guard_.page_ = nullptr;
  that.guard_.is_dirty_ = false;

  return *this;
}

void WritePageGuard::Drop() {
  if (guard_.page_ != nullptr) {
    guard_.page_->WUnlatch();
  }
  guard_.is_dirty_ = true;
  guard_.Drop();
}

WritePageGuard::~WritePageGuard() { Drop(); }  // NOLINT
