#ifndef __MEMBUF_H__
#define __MEMBUF_H__

#include <iostream>
#include <vector>

// Lock-free memory buffer
template <typename ObjType>
class MemBuf
{
  std::vector<char *>_buf;
  const size_t _init_capacity;
  size_t _capacity;
  size_t _size;
  const size_t _obj_size;
  char *_cursor;
public:
  MemBuf(const size_t n):
    _buf(), _init_capacity(n > 32 ? n : 32), _capacity(_init_capacity),
    _size(0), _obj_size(sizeof(ObjType)), _cursor(0)
  {
    _buf.push_back(new char[_obj_size * _capacity]);
    _cursor = _buf.back();
    std::cout << "construct MemBuf" << std::endl;
  }
  ~MemBuf()
  {
    for(std::vector<char *>::iterator it = _buf.begin(); it != _buf.end(); ++it)
      delete [](*it);
    _capacity = 0;
    _size = 0;
    std::cout << "destruct MemBuf" << std::endl;
  }
public:
  // allocate memory for one object: placement new
  char *allocate()
  {
    ++_size;
    char *old_cursor = 0;
    if (_size <= _capacity)
    {
      old_cursor = _cursor;
    }
    else // enlarge the buffer
    {
      _buf.push_back(new char[_obj_size * _capacity]);
      _capacity += _capacity;
      old_cursor = _cursor = _buf.back();
      std::cout << "enlarge MemBuf to " << _capacity << std::endl;
    }
    _cursor += _obj_size;
    return old_cursor;
  }
};

#endif  // __MEMBUF_H__
