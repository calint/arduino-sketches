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
template <typename Type, const unsigned Size, typename IxType> class o1store {
  Type *all_ = nullptr;
  IxType *free_ = nullptr;
  IxType free_ix_ = 0;
  IxType *alloc_ = nullptr;
  IxType alloc_ix_ = 0;
  IxType *del_ = nullptr;
  IxType del_ix_ = 0;

public:
  o1store() {
    all_ = (Type *)calloc(Size, sizeof(Type));
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

  // ~o1store() {
  //   free(all_);
  //   free(free_);
  //   free(alloc_);
  //   free(del_);
  // }

  // allocates an instance
  auto allocate() -> Type & {
    if (free_ix_ == Size) {
      Serial.printf("!!! o1store: allocate overrun\n");
      while (true)
        ;
    }
    IxType ix = free_[free_ix_];
    alloc_[alloc_ix_] = ix;
    Type &inst = all_[ix];
    inst.alloc_ix = alloc_ix_;
    free_ix_++;
    alloc_ix_++;
    return inst;
  }

  // adds instance to a list that is applied with 'apply_free()'
  void free(const Type &spr) {
    if (del_ix_ == Size) {
      Serial.printf("!!! o1store: free overrun\n");
      while (true)
        ;
    }
    del_[del_ix_++] = alloc_[spr.alloc_ix];
  }

  // de-allocates the instances that have been freed
  void apply_free() {
    IxType *it = del_;
    for (unsigned i = 0; i < del_ix_; i++, it++) {
      Type &inst_deleted = all_[*it];
      IxType inst_ix_to_move = alloc_[alloc_ix_ - 1];
      Type &inst_to_move = all_[inst_ix_to_move];
      inst_to_move.alloc_ix = inst_deleted.alloc_ix;
      alloc_[inst_deleted.alloc_ix] = inst_ix_to_move;
      alloc_ix_--;
      free_ix_--;
      free_[free_ix_] = *it;
      inst_deleted.img = nullptr;
    }
    del_ix_ = 0;
  }

  // returns pointer to list of allocated instances
  inline auto get_allocated_list() -> IxType * { return alloc_; }

  // returns size of list of allocated instances
  inline auto get_allocated_list_len() -> IxType { return alloc_ix_; }

  // returns object from 'all' list at index 'ix'
  inline auto get(IxType ix) -> Type & { return all_[ix]; }

  // returns the list with all pre-allocated instances
  inline auto get_all_list() -> Type * { return all_; }

  // returns the size of 'all' list
  constexpr auto size() -> unsigned { return Size; }

  // returns the size in bytes of heap allocated memory
  constexpr auto data_size_B() -> size_t {
    return Size * sizeof(Type) + 3 * Size * sizeof(IxType);
  }
};