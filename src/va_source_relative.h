#pragma once

#include "al_source_relative.h"

using namespace godot;

namespace va_godot
{
class VAWorld;
}

class VASourceRelative : public ALSourceRelative
{
    GDCLASS(VASourceRelative, ALSourceRelative);

private:
    va_godot::VAWorld *va_world = nullptr;

    // Set while _enter_tree found no VAWorld yet (see VAEmitter's identical waiting_for_world field).
    bool waiting_for_world = false;

    void retry_find_va_world(Node *node);

protected:
    static void _bind_methods();

public:
    VASourceRelative();
    ~VASourceRelative();

    void _enter_tree() override;
    void _exit_tree() override;

    bool play() override;
};
