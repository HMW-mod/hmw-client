#ifndef NOTIFICATION_HPP
#define NOTIFICATION_HPP

#include <string>
#include <vector>
#include <chrono>
#include <imgui.h>

enum class NotificationType {
    Info,
    Debug,
    Error,
    Success
};

class Notification {
public:
    Notification(const std::string& message, float duration, NotificationType type);

    bool isExpired() const;
    float getFadeAlpha() const;
    const std::string& getMessage() const;
    const ImVec4& getColor() const;
    NotificationType getType() const;
    float getAnimationProgress() const;

    static ImVec4 getColorForType(NotificationType type);
    const char* getIconForType(NotificationType type) const;

private:
    std::string message;
    float duration;
    NotificationType type;
    std::chrono::high_resolution_clock::time_point start_time;
    ImVec4 color;
};

class NotificationManager {
public:
    void addNotification(const std::string& message, NotificationType type, float duration);
    void render();

private:
    void renderNotification(const Notification& notification, float x, float y, float alpha);

    std::vector<Notification> notifications;
};

#endif // NOTIFICATION_HPP
