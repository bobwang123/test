#ifndef __CONSTS_H__
#define __CONSTS_H__

class Consts
{
  static const double _DOUBLE_MAX;
public:
  static const double DOUBLE_NONE;
  static inline bool is_none(const double v) { return v < -_DOUBLE_MAX; }
  static const double DOUBLE_INF;
  static inline bool is_inf(const double v) { return _DOUBLE_MAX < v; }
};

#endif /* __CONSTS_H__ */

