#pragma once

#include "al_source_relative.h"

using namespace godot;

namespace va_godot
{
class VAWorld;
}

class VASourceAmbient : public ALSourceRelative
{
    GDCLASS(VASourceAmbient, ALSourceRelative);

private:
    va_godot::VAWorld *va_world = nullptr;
    bool played = false;

    bool waiting_for_world = false;

    void retry_find_va_world(Node *node);

protected:
    static void _bind_methods();

public:
    VASourceAmbient();
    ~VASourceAmbient();

    void _enter_tree() override;
    void _exit_tree() override;
    void _process(double delta) override;

    bool play() override;
};
