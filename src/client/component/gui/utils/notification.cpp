#include <std_include.hpp>
#include "../themes/style.hpp"
#include "notification.hpp"
#include <chrono>
#include <cmath>
#define M_PI 3.14159265358979323846

Notification::Notification(const std::string& message, float duration, NotificationType type)
	: message(message), duration(duration), type(type) {
	start_time = std::chrono::high_resolution_clock::now();
	color = getColorForType(type);
}

bool Notification::isExpired() const {
	auto now = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration<float>(now - start_time).count();
	return elapsed >= duration;
}

float Notification::getFadeAlpha() const {
	auto now = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration<float>(now - start_time).count();
	if (elapsed >= duration - 0.5f) {
		return std::max(0.0f, 1.0f - (elapsed - (duration - 0.5f)) / 0.5f);
	}
	return 1.0f;
}

const std::string& Notification::getMessage() const { return message; }
const ImVec4& Notification::getColor() const { return color; }
NotificationType Notification::getType() const { return type; }

float Notification::getAnimationProgress() const {
	auto now = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration<float>(now - start_time).count();
	float progress = std::min(elapsed / 0.5f, 1.0f);
	return 0.5f * (1.0f - std::cos(progress * M_PI));
}



const char* Notification::getIconForType(NotificationType type) const {
	switch (type) {
	case NotificationType::Info:    return reinterpret_cast<const char*>(u8"\uf05a");
	case NotificationType::Debug:   return reinterpret_cast<const char*>(u8"\uf188");
	case NotificationType::Error:   return reinterpret_cast<const char*>(u8"\uf057");
	case NotificationType::Success: return reinterpret_cast<const char*>(u8"\uf058");
	default: return reinterpret_cast<const char*>(u8"\uf128");
	}
}

ImVec4 Notification::getColorForType(NotificationType type) {
	switch (type) {
	case NotificationType::Info:    return ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
	case NotificationType::Debug:   return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
	case NotificationType::Error:   return ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
	case NotificationType::Success: return ImVec4(0.2f, 1.0f, 0.4f, 1.0f);
	default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void NotificationManager::addNotification(const std::string& message, NotificationType type, float duration) {
	notifications.emplace_back(message, duration, type);
}

void NotificationManager::render() {
	float xOffset = ImGui::GetIO().DisplaySize.x;
	float yOffset = 50.0f;

	for (auto it = notifications.begin(); it != notifications.end();) {
		if (it->isExpired()) {
			it = notifications.erase(it);
		}
		else {
			float progress = it->getAnimationProgress();
			float animatedX = xOffset - (progress * 260.0f);
			renderNotification(*it, animatedX, yOffset, it->getFadeAlpha());
			yOffset += 60.0f;
			++it;
		}
	}
}

void NotificationManager::renderNotification(const Notification& notification, float x, float y, float alpha) {
	ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(300.0f, 60.0f));
	ImGui::SetNextWindowBgAlpha(0.9f * alpha);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
	ImGui::Begin("##Notification", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav);

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetWindowPos();
	ImVec2 size = ImGui::GetWindowSize();
	ImVec4 color = notification.getColor();
	color.w *= alpha;

	ImGui::PushFont(iconFontBrands);
	ImVec2 icon_size = ImGui::CalcTextSize(notification.getIconForType(notification.getType()));
	ImGui::SetCursorPos(ImVec2(10, (size.y - icon_size.y) / 2.0f));
	ImGui::TextColored(ImVec4(color.x, color.y, color.z, alpha), "%s", notification.getIconForType(notification.getType()));
	ImGui::PopFont();

	ImVec2 text_size = ImGui::CalcTextSize(notification.getMessage().c_str());
	ImVec2 text_pos = ImVec2(
		10 + icon_size.x + 10,
		(size.y - text_size.y) / 2.0f
	);
	ImGui::SetCursorPos(text_pos);
	ImGui::TextColored(ImVec4(color.x, color.y, color.z, alpha), "%s", notification.getMessage().c_str());

	ImGui::End();
	ImGui::PopStyleVar();
}