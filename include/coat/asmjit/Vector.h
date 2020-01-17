#ifndef COAT_ASMJIT_VECTOR_H_
#define COAT_ASMJIT_VECTOR_H_

#include <cassert>

#include "Ptr.h"


namespace coat {

template<typename T, unsigned width>
struct Vector<::asmjit::x86::Compiler,T,width> final {
	using F = ::asmjit::x86::Compiler;
	using value_type = T;
	
	static_assert(sizeof(T)==1 || sizeof(T)==2 || sizeof(T)==4 || sizeof(T)==8,
		"only plain arithmetic types supported of sizes: 1, 2, 4 or 8 bytes");
	static_assert(std::is_signed_v<T> || std::is_unsigned_v<T>,
		"only plain signed or unsigned arithmetic types supported");
	static_assert(sizeof(T)*width == 128/8 || sizeof(T)*width == 256/8,
		"only 128-bit and 256-bit vector instructions supported at the moment");

	using reg_type = std::conditional_t<sizeof(T)*width==128/8,
						::asmjit::x86::Xmm,
						::asmjit::x86::Ymm // only these two are allowed
					>;
	reg_type reg;


	F &cc; //FIXME: pointer stored in every value type

	Vector(F &cc, const char *name="") : cc(cc){
		if constexpr(std::is_same_v<reg_type,::asmjit::x86::Xmm>){
			// 128 bit SSE
			reg = cc.newXmm(name);
		}else{
			// 256 bit AVX
			reg = cc.newYmm(name);
		}
	}

	inline unsigned getWidth() const { return width; }

	// load vector from memory, always unaligned load
	Vector &operator=(Ref<F,Value<F,T>> &&src){ load(std::move(src)); return *this; }
	// load vector from memory, always unaligned load
	void load(Ref<F,Value<F,T>> &&src){
		if constexpr(std::is_same_v<reg_type,::asmjit::x86::Xmm>){
			// 128 bit SSE
			src.mem.setSize(16); // change to xmmword
			cc.movdqu(reg, src);
		}else{
			// 256 bit AVX
			src.mem.setSize(32); // change to ymmword
			cc.vmovdqu(reg, src);
		}
	}

	// unaligned store
	void store(Ref<F,Value<F,T>> &&dest) const {
		if constexpr(std::is_same_v<reg_type,::asmjit::x86::Xmm>){
			// 128 bit SSE
			dest.mem.setSize(16); // change to xmmword
			cc.movdqu(dest, reg);
		}else{
			// 256 bit AVX
			dest.mem.setSize(32); // change to ymmword
			cc.vmovdqu(dest, reg);
		}
	}
	//TODO: aligned load & store

	Vector &operator+=(const Vector &other){
		if constexpr(std::is_same_v<reg_type,::asmjit::x86::Xmm>){
			// 128 bit SSE
			switch(sizeof(T)){
				case 1: cc.paddb(reg, other.reg); break;
				case 2: cc.paddw(reg, other.reg); break;
				case 4: cc.paddd(reg, other.reg); break;
				case 8: cc.paddq(reg, other.reg); break;
			}
		}else{
			// 256 bit AVX
			switch(sizeof(T)){
				case 1: cc.vpaddb(reg, reg, other.reg); break;
				case 2: cc.vpaddw(reg, reg, other.reg); break;
				case 4: cc.vpaddd(reg, reg, other.reg); break;
				case 8: cc.vpaddq(reg, reg, other.reg); break;
			}
		}
		return *this;
	}
	Vector &operator+=(Ref<F,Value<F,T>> &&other){
		if constexpr(std::is_same_v<reg_type,::asmjit::x86::Xmm>){
			// 128 bit SSE
			other.mem.setSize(16); // change to xmmword
			switch(sizeof(T)){
				case 1: cc.paddb(reg, other); break;
				case 2: cc.paddw(reg, other); break;
				case 4: cc.paddd(reg, other); break;
				case 8: cc.paddq(reg, other); break;
			}
		}else{
			// 256 bit AVX
			other.mem.setSize(32); // change to ymmword
			switch(sizeof(T)){
				case 1: cc.vpaddb(reg, reg, other); break;
				case 2: cc.vpaddw(reg, reg, other); break;
				case 4: cc.vpaddd(reg, reg, other); break;
				case 8: cc.vpaddq(reg, reg, other); break;
			}
		}
		return *this;
	}

	Vector &operator>>=(int amount){
		static_assert(sizeof(T) > 1, "shift does not support byte element size");
		if constexpr(std::is_same_v<reg_type,::asmjit::x86::Xmm>){
			// 128 bit SSE
			switch(sizeof(T)){
				case 2: cc.psrlw(reg, amount); break;
				case 4: cc.psrld(reg, amount); break;
				case 8: cc.psrlq(reg, amount); break;
			}
		}else{
			// 256 bit AVX
			switch(sizeof(T)){
				case 2: cc.vpsrlw(reg, reg, amount); break;
				case 4: cc.vpsrld(reg, reg, amount); break;
				case 8: cc.vpsrlq(reg, reg, amount); break;
			}
		}
		return *this;
	}

	Vector &operator/=(int amount){
		if(is_power_of_two(amount)){
			operator>>=(clog2(amount));
		}else{
			//TODO
			assert(false);
		}
		return *this;
	}
};


template<int width, typename T>
Vector<::asmjit::x86::Compiler,T,width> make_vector(::asmjit::x86::Compiler &cc, Ref<::asmjit::x86::Compiler,Value<::asmjit::x86::Compiler,T>> &&src){
	Vector<::asmjit::x86::Compiler,T,width> v(cc);
	v = std::move(src);
	return v;
}

} // namespace

#endif