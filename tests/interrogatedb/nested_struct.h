struct Parent {
__published:
  struct InlineChild {
  __published:
    int member;
  };

  struct OutOfLineChild;

  struct IncompleteChild;

  struct ForwardDeclaredInlineChild;
  struct ForwardDeclaredInlineChild {
  __published:
    int member;
  };
};

struct Parent::OutOfLineChild {
__published:
  int member;
};
