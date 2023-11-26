#pragma once
//
// implements a O(1) store of objects
//
// * Type is object type. 'Type' must contain public field '<IxType> alloc_ix'
// * Size is number of pre-allocated objects
// * IxType is type used to index in lists and should be unsigned with a bit
//   width that fits 'Size'
// * StoreId is for debugging
// * InstanceSizeInBytes is custom size of instance
//   used to fit largest object in an object hierarchy in lack of std::variant
//
// example:
//   Type = sprite, Size = 255, IxType = uint8_t gives 255 allocatable sprites
//   note. size should be at most the number that fits in IxType
//         in example uint8_t fits 255 indexes
//
// note. no destructor since life-time is program life-time
//
template <typename Type, const unsigned Size, typename IxType,
          const unsigned StoreId = 0, const unsigned InstanceSizeInBytes = 0>
class o1store {
  Type *all_ = nullptr;
  IxType *free_bgn_ = nullptr;
  IxType *free_ptr_ = nullptr;
  IxType *free_end_ = nullptr;
  IxType *alloc_bgn_ = nullptr;
  IxType *alloc_ptr_ = nullptr;
  IxType alloc_ix_ = 0;
  IxType *del_bgn_ = nullptr;
  IxType *del_ptr_ = nullptr;
  IxType *del_end_ = nullptr;

public:
  o1store() {
    if constexpr (InstanceSizeInBytes) {
      all_ = (Type *)calloc(Size, InstanceSizeInBytes);
    } else {
      all_ = (Type *)calloc(Size, sizeof(Type));
    }
    free_ptr_ = free_bgn_ = (IxType *)calloc(Size, sizeof(IxType));
    free_end_ = free_bgn_ + Size;
    alloc_ptr_ = alloc_bgn_ = (IxType *)calloc(Size, sizeof(IxType));
    del_ptr_ = del_bgn_ = (IxType *)calloc(Size, sizeof(IxType));
    del_end_ = del_bgn_ + Size;
    if (!all_ or !free_bgn_ or !alloc_bgn_ or !del_bgn_) {
      Serial.printf("!!! o1store %u: could not allocate arrays\n", StoreId);
      while (true)
        ;
    }
    IxType i = 0;
    for (IxType *it = free_bgn_; it < free_end_; i++, it++) {
      *it = i;
    }
  }

  // virtual ~o1store() {
  //   free(all_);
  //   free(free_);
  //   free(alloc_);
  //   free(del_);
  // }

  // returns true if allocatable instances available
  inline auto can_allocate() -> bool { return free_ptr_ < free_end_; }

  // allocates an instance
  auto allocate_instance() -> Type * {
    if (free_ptr_ >= free_end_) {
      return nullptr;
    }
    IxType ix = *free_ptr_;
    free_ptr_++;
    *alloc_ptr_ = ix;
    alloc_ptr_++;
    Type *inst = instance(ix);
    inst->alloc_ix = alloc_ix_;
    alloc_ix_++;
    return inst;
  }

  // adds instance to a list that is applied with 'apply_free()'
  void free_instance(const Type *inst) {
    if (del_ptr_ >= del_end_) {
      Serial.printf("!!! o1store %u: free overrun [alloc_ix=%u]\n", StoreId,
                    inst->alloc_ix);
      while (true)
        ;
    }
    *del_ptr_ = alloc_bgn_[inst->alloc_ix];
    del_ptr_++;
  }

  // de-allocates the instances that have been freed
  void apply_free() {
    for (IxType *it = del_bgn_; it < del_ptr_; it++) {
      Type *inst_deleted = instance(*it);
      alloc_ptr_--;
      IxType inst_ix_to_move = *alloc_ptr_;
      Type *inst_to_move = instance(inst_ix_to_move);
      inst_to_move->alloc_ix = inst_deleted->alloc_ix;
      alloc_bgn_[inst_deleted->alloc_ix] = inst_ix_to_move;
      free_ptr_--;
      *free_ptr_ = *it;
    }
    alloc_ix_ -= (del_ptr_ - del_bgn_);
    del_ptr_ = del_bgn_;
  }

  // returns pointer to list of allocated instances
  inline auto allocated_list() -> IxType * { return alloc_bgn_; }

  // returns size of list of allocated instances
  inline auto allocated_list_len() -> IxType { return alloc_ix_; }

  // returns the list with all pre-allocated instances
  inline auto all_list() -> Type * { return all_; }

  // returns the size of 'all' list
  constexpr auto all_list_len() -> unsigned { return Size; }

  // returns instance from 'all' list at index 'ix'
  inline auto instance(IxType ix) -> Type * {
    if constexpr (!InstanceSizeInBytes) {
      return &all_[ix];
    }
    // note. if instance size is specified do pointer shenanigans
    return (Type *)((void *)all_ + InstanceSizeInBytes * ix);
  }

  // returns the size in bytes of allocated heap memory
  constexpr auto allocated_data_size_B() -> size_t {
    if constexpr (InstanceSizeInBytes) {
      return Size * InstanceSizeInBytes + 3 * Size * sizeof(IxType);
    }
    return Size * sizeof(Type) + 3 * Size * sizeof(IxType);
  }
};