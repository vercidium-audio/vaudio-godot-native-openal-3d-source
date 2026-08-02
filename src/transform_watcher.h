#pragma once

#include <godot_cpp/classes/node3d.hpp>
#include <functional>

using namespace godot;

// A lightweight child node that fires a callback whenever its parent's global
// transform changes. Attach to any Node3D, then set on_transform_changed.
// set_notify_transform(true) is called here (not on the parent) so only this
// node receives the notification, avoiding double-processing.
class TransformWatcher : public Node3D
{
    GDCLASS(TransformWatcher, Node3D);

private:
    std::function<void()> on_transform_changed;

protected:
    static void _bind_methods();

public:
    void set_on_transform_changed(std::function<void()> callback);

    void _ready() override;
    void _notification(int what);
};
