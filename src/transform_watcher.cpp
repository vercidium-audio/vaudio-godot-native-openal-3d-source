#include "transform_watcher.h"

void TransformWatcher::_bind_methods()
{
}

void TransformWatcher::set_on_transform_changed(std::function<void()> callback)
{
    on_transform_changed = callback;
}

void TransformWatcher::_ready()
{
    set_notify_transform(true);
}

void TransformWatcher::_notification(int what)
{
    if (what == NOTIFICATION_TRANSFORM_CHANGED && on_transform_changed)
    {
        on_transform_changed();
    }
}
