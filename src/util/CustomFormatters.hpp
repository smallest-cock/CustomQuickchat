#pragma once
#include "RLSDK/RLSDK_w_pch_includes/SDK_HEADERS/Core_structs.hpp"
#include "RLSDK/RLSDK_w_pch_includes/SDK_HEADERS/Core_classes.hpp"

template <>
struct std::formatter<FString> {
	constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
	template <typename FormatContext>
	auto format(const FString &str, FormatContext &ctx) const {
		return std::format_to(ctx.out(), "{}", str.ToString());
	}
};

template <>
struct std::formatter<FName> {
	constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
	template <typename FormatContext>
	auto format(const FName &str, FormatContext &ctx) const {
		return std::format_to(ctx.out(), "{}", str.ToString());
	}
};

template <UObjectOrDerived T>
struct std::formatter<T *> {
	constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
	template <typename FormatContext>
	auto format(T *obj, FormatContext &ctx) const {
		return std::format_to(ctx.out(), "0x{:X}", reinterpret_cast<uintptr_t>(obj));
	}
};

template <>
struct std::formatter<FVector> {
	constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
	template <typename FormatContext>
	auto format(const FVector &vec, FormatContext &ctx) const {
		return std::format_to(ctx.out(), "[X:{:.2f}-Y:{:.2f}-Z:{:.2f}]", vec.X, vec.Y, vec.Z);
	}
};

template <>
struct std::formatter<FLinearColor> {
	constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
	template <typename FormatContext>
	auto format(const FLinearColor &col, FormatContext &ctx) const {
		return std::format_to(ctx.out(), "[R:{:.2f}-G:{:.2f}-B:{:.2f}-A:{:.2f}]", col.R, col.G, col.B, col.A);
	}
};

template <>
struct std::formatter<FColor> {
	constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
	template <typename FormatContext>
	auto format(const FColor &col, FormatContext &ctx) const {
		return std::format_to(ctx.out(), "[R:{}-G:{}-B:{}]", col.R, col.G, col.B, col.A);
	}
};

template <>
struct std::formatter<FRotator> {
	constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }
	template <typename FormatContext>
	auto format(const FRotator &rot, FormatContext &ctx) const {
		return std::format_to(ctx.out(), "[Pitch:{}-Yaw:{}-Roll:{}]", rot.Pitch, rot.Yaw, rot.Roll);
	}
};
