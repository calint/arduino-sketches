#pragma once
//
// implements a O(1) store of objects
//
// * Type is object type. 'Type' must contain public field '<IxType> alloc_ix'
// * Size is number of pre-allocated objects
// * IxType is type used to index in lists and should be unsigned with a bit
//   width that fits 'Size'
//
// example:
//   Type = sprite, Size = 255, IxType = uint8_t gives 255 allocatable sprites
//   note. size should be at most the number that fits in IxType
//         in example uint8_t fits 255 indexes
//
// note. no destructor since life-time is program life-time
//
template <typename Type, const unsigned Size, typename IxType,
          const char StoreId = 0, const unsigned InstanceSizeInBytes = 0>
class o1store {
  Type *all_ = nullptr;
  IxType *free_ = nullptr;
  IxType free_ix_ = 0;
  IxType *alloc_ = nullptr;
  IxType alloc_ix_ = 0;
  IxType *del_ = nullptr;
  IxType del_ix_ = 0;

public:
  o1store() {
    if constexpr (InstanceSizeInBytes) {
      all_ = (Type *)calloc(Size, InstanceSizeInBytes);
    } else {
      all_ = (Type *)calloc(Size, sizeof(Type));
    }
    free_ = (IxType *)calloc(Size, sizeof(IxType));
    alloc_ = (IxType *)calloc(Size, sizeof(IxType));
    del_ = (IxType *)calloc(Size, sizeof(IxType));
    if (!all_ or !free_ or !alloc_ or !del_) {
      Serial.printf("!!! o1store: could not allocate arrays\n");
      while (true)
        ;
    }
    for (unsigned i = 0; i < Size; i++) {
      free_[i] = i;
    }
  }

  // virtual ~o1store() {
  //   free(all_);
  //   free(free_);
  //   free(alloc_);
  //   free(del_);
  // }

  // returns true if allocatable instances available
  auto can_allocate() -> bool { return free_ix_ < Size; }

  // allocates an instance
  auto allocate_instance() -> Type & {
    if (free_ix_ == Size) {
      Serial.printf("!!! o1store %u: allocate overrun\n", StoreId);
      while (true)
        ;
    }
    IxType ix = free_[free_ix_];
    alloc_[alloc_ix_] = ix;
    Type &inst = instance(ix);
    inst.alloc_ix = alloc_ix_;
    // Serial.printf("o1store %u allocate %u\n", StoreId, inst.alloc_ix);
    free_ix_++;
    alloc_ix_++;
    return inst;
  }

  // adds instance to a list that is applied with 'apply_free()'
  void free_instance(const Type &inst) {
    // Serial.printf("free instance: %u\n", inst.alloc_ix);
    if (del_ix_ == Size) {
      Serial.printf("!!! o1store %u: free overrun\n", StoreId);
      while (true)
        ;
    }
    del_[del_ix_++] = alloc_[inst.alloc_ix];
  }

  // de-allocates the instances that have been freed
  void apply_free() {
    IxType *it = del_;
    for (unsigned i = 0; i < del_ix_; i++, it++) {
      Type &inst_deleted = instance(*it);
      IxType inst_ix_to_move = alloc_[alloc_ix_ - 1];
      Type &inst_to_move = instance(inst_ix_to_move);
      inst_to_move.alloc_ix = inst_deleted.alloc_ix;
      alloc_[inst_deleted.alloc_ix] = inst_ix_to_move;
      alloc_ix_--;
      free_ix_--;
      free_[free_ix_] = *it;
    }
    del_ix_ = 0;
  }

  // returns pointer to list of allocated instances
  inline auto allocated_list() -> IxType * { return alloc_; }

  // returns size of list of allocated instances
  inline auto allocated_list_len() -> IxType { return alloc_ix_; }

  // returns the list with all pre-allocated instances
  inline auto all_list() -> Type * { return all_; }

  // returns the size of 'all' list
  constexpr auto all_list_len() -> unsigned { return Size; }

  // returns instance from 'all' list at index 'ix'
  inline auto instance(IxType ix) -> Type & {
    if constexpr (!InstanceSizeInBytes) {
      return all_[ix];
    }
    // note. if instance size is specified do pointer shenanigans
    return *(Type *)((void *)all_ + InstanceSizeInBytes * ix);
  }

  // returns the size in bytes of allocated heap memory
  constexpr auto allocated_data_size_B() -> size_t {
    if constexpr (InstanceSizeInBytes) {
      return Size * InstanceSizeInBytes + 3 * Size * sizeof(IxType);
    }
    return Size * sizeof(Type) + 3 * Size * sizeof(IxType);
  }
};