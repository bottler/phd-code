#ifndef JR_BCH_H
#define JR_BCH_H
#include <array>
template<size_t d, size_t m> class SigLength;
template<size_t d> using Segment = std::array<double,d>;
template<size_t d, size_t m> using LogSignature=std::array<double,SigLength<d,m>::value>;
template<size_t d> using FSegment = std::array<float,d>;
template<size_t d, size_t m> using FLogSignature=std::array<float,SigLength<d,m>::value>;
template<size_t d, size_t m>
void joinSegmentToSignatureInPlace(LogSignature<d,m>& a, const Segment<d>& b);
template<size_t d, size_t m>
void joinSegmentToSignatureInPlace(FLogSignature<d,m>& a, const FSegment<d>& b);
template<> class SigLength<2,1>{public: enum {value = 2}; };
template<>
void joinSegmentToSignatureInPlace<2,1>(LogSignature<2,1>& a, const Segment<2>& b);
template<>
void joinSegmentToSignatureInPlace<2,1>(FLogSignature<2,1>& a, const FSegment<2>& b);
template<> class SigLength<2,2>{public: enum {value = 3}; };
template<>
void joinSegmentToSignatureInPlace<2,2>(LogSignature<2,2>& a, const Segment<2>& b);
template<>
void joinSegmentToSignatureInPlace<2,2>(FLogSignature<2,2>& a, const FSegment<2>& b);
template<> class SigLength<2,3>{public: enum {value = 5}; };
template<>
void joinSegmentToSignatureInPlace<2,3>(LogSignature<2,3>& a, const Segment<2>& b);
template<>
void joinSegmentToSignatureInPlace<2,3>(FLogSignature<2,3>& a, const FSegment<2>& b);
template<> class SigLength<2,4>{public: enum {value = 8}; };
template<>
void joinSegmentToSignatureInPlace<2,4>(LogSignature<2,4>& a, const Segment<2>& b);
template<>
void joinSegmentToSignatureInPlace<2,4>(FLogSignature<2,4>& a, const FSegment<2>& b);
template<> class SigLength<2,5>{public: enum {value = 14}; };
template<>
void joinSegmentToSignatureInPlace<2,5>(LogSignature<2,5>& a, const Segment<2>& b);
template<>
void joinSegmentToSignatureInPlace<2,5>(FLogSignature<2,5>& a, const FSegment<2>& b);
template<> class SigLength<2,6>{public: enum {value = 23}; };
template<>
void joinSegmentToSignatureInPlace<2,6>(LogSignature<2,6>& a, const Segment<2>& b);
template<>
void joinSegmentToSignatureInPlace<2,6>(FLogSignature<2,6>& a, const FSegment<2>& b);
#endif
