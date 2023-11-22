#pragma once
//
// implements a O(1) store of objects
//
// Type is object type. Type must contain public field 'IxType alloc_ix'
// Size is number of pre-allocated objects
// IxType is type used to index in lists
// index_0_reserved is true if object at index 0 is un-allocatable / reserved
//
// example:
//   Type = sprite, Size = 255, IxType = uint8_t gives 255 allocatable objects
//   note. size should be maximum the number that fits in IxType
//         in example uint8_t fits 255 indexes
//         if index_0_reserved than there are 254 allocatable objects
//
template <typename Type, const unsigned Size, typename IxType,
          const bool index_0_reserved = false>
class o1store {
  Type *all_ = nullptr;
  IxType *free_ = nullptr;
  IxType free_ix_ = index_0_reserved ? 1 : 0;
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

  void free(const Type &spr) {
    if (del_ix_ == Size) {
      Serial.printf("!!! o1store: free overrun\n");
      while (true)
        ;
    }
    del_[del_ix_++] = alloc_[spr.alloc_ix];
  }

  void apply_free() {
    IxType *del = del_;
    for (unsigned i = 0; i < del_ix_; i++, del++) {
      Type &inst_deleted = all_[*del];
      IxType inst_ix_to_move = alloc_[alloc_ix_ - 1];
      Type &inst_to_move = all_[inst_ix_to_move];
      inst_to_move.alloc_ix = inst_deleted.alloc_ix;
      alloc_[inst_deleted.alloc_ix] = inst_ix_to_move;
      alloc_ix_--;
      free_ix_--;
      free_[free_ix_] = *del;
      inst_deleted.img = nullptr;
    }
    del_ix_ = 0;
  }

  inline auto get_allocated_list() -> IxType * { return alloc_; }

  inline auto get_allocated_list_len() -> IxType { return alloc_ix_; }

  inline auto get(IxType ix) -> Type & { return all_[ix]; }

  constexpr inline auto size() -> unsigned { return Size; }

  constexpr inline auto data_size_B() -> size_t {
    return Size * sizeof(Type) + 3 * Size * sizeof(IxType);
  }
};