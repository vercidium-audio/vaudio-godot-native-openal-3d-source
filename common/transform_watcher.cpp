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
    // this is called here (not on the parent node) so only this node receives the notification, avoiding double-processing
    set_notify_transform(true);
}

void TransformWatcher::_notification(int what)
{
    if (what != NOTIFICATION_TRANSFORM_CHANGED)
        return;

    // Godot only re-queues NOTIFICATION_TRANSFORM_CHANGED for a Node2D/Node3D once its cached global transform has been read again. Without this line, only the first move after the node enters the tree ever fires
    get_global_transform();

    if (on_transform_changed)
        on_transform_changed();
}
