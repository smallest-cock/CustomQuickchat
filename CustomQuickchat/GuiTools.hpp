#pragma once
#include "pch.h"
#include "Components/Components/Utils.hpp"


namespace GUI
{
	void open_link(const char* url);
	void open_link(const wchar_t* url);

	void ClickableLink(const char* label, const char* url, const ImVec4& textColor = ImVec4(1, 1, 1, 1), ImVec2 size = ImVec2(0, 0));

	void Spacing(int amount = 1);
	void SameLineSpacing_relative(float horizontalSpacingPx);
	void SameLineSpacing_absolute(float horizontalSpacingPx);


	static constexpr float footer_icon_height =		25.0f;

	static constexpr const wchar_t* discord_link = L"https://discord.gg/d5ahhQmJbJ";
	static constexpr const char* discord_desc =		"Need help? Join the discord";

	static constexpr const wchar_t* youtube_link =	L"https://www.youtube.com/@SSLowRL";
	static constexpr const char* youtube_desc =		"YouTube page";


	struct FooterAssets
	{
		fs::path github_img_path;
		fs::path discord_img_path;
		fs::path youtube_img_path;

		bool all_assets_exist()
		{
			if (!fs::exists(github_img_path))
			{
				LOG("[ERROR] File not found: {}", github_img_path.string());
				return false;
			}

			if (!fs::exists(discord_img_path))
			{
				LOG("[ERROR] File not found: {}", discord_img_path.string());
				return false;
			}

			if (!fs::exists(youtube_img_path))
			{
				LOG("[ERROR] File not found: {}", youtube_img_path.string());
				return false;
			}

			return true;
		}
	};

	struct ImageLink
	{
		const wchar_t* link;
		const char* tooltip;
		std::shared_ptr<ImageWrapper> image;
		ImVec2 image_size;
		float target_height = 25.0f;

		ImageLink() = delete;
		ImageLink(std::shared_ptr<ImageWrapper> img, float desired_height = 40.0f) : image(img) { init(); }

		ImageLink(std::shared_ptr<ImageWrapper> img, const wchar_t* url, const char* tooltip = "", float desired_height = 40.0f):
			image(img),
			link(url),
			tooltip(tooltip),
			target_height(desired_height)
		{
			init();
		}

		ImageLink(const fs::path img_path, const wchar_t* url, const char* tooltip = "", float desired_height = 40.0f):
			image(std::make_shared<ImageWrapper>(img_path, false, true)),
			link(url),
			tooltip(tooltip),
			target_height(desired_height)
		{
			init();
		}


		// this doesnt seem to do shit -_-
		void init()
		{
			if (!image) return;

			auto img_loaded_callback = [this](bool loaded)
			{
				std::string_view msg = loaded ? "Loaded ImGui image :)" : "Failed to load ImGui image :(";
				LOG(msg);

				if (!loaded || !image) return;

				LOG("image ImGui texture: {}", Format::ToHexString(reinterpret_cast<uintptr_t>(image->GetImGuiTex())));

				auto size = image->GetSizeF();
				image_size = { size.X, size.Y };

				LOG("X:{} - Y:{}", image_size.x, image_size.y);

				scale_to_desired_height();
			};

			image->LoadForImGui(img_loaded_callback);
			LOG("Ran 'image->LoadForImGui(img_loaded_callback)'");
		}

		bool size_is_zero()
		{
			return image_size.x == 0 || image_size.y == 0;
		}

		void update_image_size(bool scale_to_target_h = true)
		{
			auto old = image_size;

			auto size = image->GetSizeF();
			image_size = { size.X, size.Y };

			LOG("Updated image size...");
			LOG("OLD\tX:{} - Y:{}", old.x, old.y);
			
			if (scale_to_target_h)
			{
				scale_to_desired_height();
			}

			LOG("NEW\tX:{} - Y:{}", image_size.x, image_size.y);
		}

		void scale_to_desired_height()
		{
			Vector2F size = image->GetSizeF();

			if (size.Y <= 0) return;

			const float scale_factor = target_height / size.Y;		//		x / 100		=	target_height / og_width

			size *= scale_factor;

			image_size = { size.X, size.Y };
			
			LOG("Scaled image to desired height...");
			LOG("Target: {} --- Actual: {}", target_height, image_size.y);
		}

		void scale_to_desired_width(float target_width)
		{
			Vector2F size = image->GetSizeF();

			if (size.X <= 0) return;

			const float scale_factor = target_width / size.X;		//		x / 100		=	target_height / og_width

			size *= scale_factor;

			image_size = { size.X, size.Y };
		}


		void display(const float target_height = 25.0f)
		{
			if (!image || !image->IsLoadedForImGui()) return;

			if (auto img_tex = image->GetImGuiTex())
			{
				if (size_is_zero())
				{
					LOG("image size was zero, so scaling to desired height...");
					scale_to_desired_height();
				}

				ImGui::Image(img_tex, image_size);

				if (ImGui::IsItemHovered())
				{
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
					ImGui::SetTooltip(tooltip);
				}
				if (ImGui::IsItemClicked())
				{
					open_link(link);
				}
			}
		}
	};

	struct FooterLinks
	{
		ImageLink github;
		ImageLink discord;
		ImageLink youtube;
		float horizontal_spacing_between_imgs = 20.0f;

		float get_total_img_width() const
		{
			return github.image_size.x + discord.image_size.x + youtube.image_size.x;
		}

		bool any_img_width_zero() const
		{
			if (github.image_size.x == 0) return true;
			if (discord.image_size.x == 0) return true;
			if (youtube.image_size.x == 0) return true;
			return false;
		}

		void update_image_sizes()
		{
			github.update_image_size();
			discord.update_image_size();
			youtube.update_image_size();
		}

		void display()
		{
			if (any_img_width_zero())
			{
				LOG("Oops! A footer image had a width of 0.... finna update image sizes");
				update_image_sizes();
			}

			// calculate horizontal offset to center links
			const float total_img_width = get_total_img_width();
			if (total_img_width == 0)
			{
				LOG("Total image width is 0.... something is wrong fds$#8533*&4$Ewfe");
				update_image_sizes();
			}

			const float total_width = total_img_width + (2 * horizontal_spacing_between_imgs);

			float horizontal_offset = (ImGui::GetContentRegionAvail().x - total_width) / 2;
			horizontal_offset -= 50;	// it seems slightly shifted to the right, so subtract 50 to compensate
			ImGui::SetCursorPosX(horizontal_offset);


			// display links
			github.display();
			SameLineSpacing_relative(horizontal_spacing_between_imgs);
			discord.display();
			SameLineSpacing_relative(horizontal_spacing_between_imgs);
			youtube.display();
		}
	};


	namespace Colors
	{
		struct Color
		{
			float r, g, b, a;

			Color() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
			
			// Initialize with float values
			Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}

			Color(const ImVec4& color) : r(color.w), g(color.x), b(color.y), a(color.z) {}

			// Initialize with an FLinearColor
			Color(const FLinearColor& color) : r(color.R), g(color.G), b(color.B), a(color.A) {}

			// Initialize with a int32_t color (0xAARRGGBB format)
			Color(int32_t color)
			{
				a = ((color >> 24) & 0xFF)	/ 255.0f;	// Extract and normalize Alpha
				r = ((color >> 16) & 0xFF)	/ 255.0f;	// Extract and normalize Red
				g = ((color >> 8) & 0xFF)	/ 255.0f;	// Extract and normalize Green
				b = (color & 0xFF)			/ 255.0f;	// Extract and normalize Blue
			}

			// Initialize with an FColor
			Color(const FColor& color)
			{
				r = color.R / 255.0f; // Normalize Red
				g = color.G / 255.0f; // Normalize Green
				b = color.B / 255.0f; // Normalize Blue
				a = color.A / 255.0f; // Normalize Alpha
			}



			ImVec4 GetImGuiColor() const
			{
				return ImVec4{ r, g, b, a };
			}

			FLinearColor GetLinearColor() const
			{
				return FLinearColor{ r, g, b, a };
			}

			int32_t GetIntColor() const
			{
				FColor col = GetFColor();

				return	(static_cast<int32_t>(col.A) << 24)	|
						(static_cast<int32_t>(col.R) << 16)	|
						(static_cast<int32_t>(col.G) << 8)	|
						static_cast<int32_t>(col.B);
			}

			FColor GetFColor() const
			{
				FColor col;

				col.R = static_cast<uint8_t>(std::round(r * 255.0f));
				col.G = static_cast<uint8_t>(std::round(g * 255.0f));
				col.B = static_cast<uint8_t>(std::round(b * 255.0f));
				col.A = static_cast<uint8_t>(std::round(a * 255.0f));

				return col;
			}
		};


		extern const ImVec4 White;
		extern const ImVec4 Red;
		extern const ImVec4 Green;
		extern const ImVec4 Blue;
		
		extern const ImVec4 Yellow;
		extern const ImVec4 BlueGreen;
		extern const ImVec4 Pinkish;
		
		extern const ImVec4 LightBlue;
		extern const ImVec4 LightRed;
		extern const ImVec4 LightGreen;

		extern const ImVec4 DarkGreen;

		extern const ImVec4 Gray;
	}


	void SettingsHeader(const char* id, const char* pluginVersion, const ImVec2& size, bool showBorder = false);
	void SettingsFooter(const char* id, const ImVec2& size, std::shared_ptr<FooterLinks> footer_links, bool showBorder = false);
	void OldSettingsFooter(const char* id, const ImVec2& size, bool showBorder = false);

	void alt_settings_header(const char* text, const char* plugin_version, const ImVec4& text_color = Colors::Pinkish);
	void alt_settings_footer(const char* text, const char* url, const ImVec4& text_color = Colors::Yellow);
}