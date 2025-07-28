//FLAGS: -D__cplusplus -python-native

class Vector {
__published:
  int operator [](int n) const;
  int &operator [](int n);
};
