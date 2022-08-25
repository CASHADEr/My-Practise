namespace Cshr::Util {
template <typename Small, typename Large>
inline Small static_down_cast(const Large &large) {
  Small small = static_cast<Small>(large);
#if defined __debug__
  assert(large - small == 0);
#endif
  return small;
}
}  // namespace Cshr::Util