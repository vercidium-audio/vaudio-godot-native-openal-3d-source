#include "va_source_leech.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "va_emitter.h"
#include "va_engine_util.h"
#include "va_world.h"
#include "va_world_lookup.h"

void VASourceLeech::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("get_play_when_raytracing_completes"), &VASourceLeech::get_play_when_raytracing_completes);
    ClassDB::bind_method(D_METHOD("set_play_when_raytracing_completes", "value"), &VASourceLeech::set_play_when_raytracing_completes);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "play_when_raytracing_completes"), "set_play_when_raytracing_completes", "get_play_when_raytracing_completes");

    // Read-only muffling stats - no ADD_PROPERTY, same rationale as VASource's.
    ClassDB::bind_method(D_METHOD("get_muffling_gain_lf"), &VASourceLeech::get_muffling_gain_lf);
    ClassDB::bind_method(D_METHOD("get_muffling_gain_hf"), &VASourceLeech::get_muffling_gain_hf);
    ClassDB::bind_method(D_METHOD("is_raytraced"), &VASourceLeech::is_raytraced);
}

VASourceLeech::VASourceLeech()
{
}

VASourceLeech::~VASourceLeech()
{
}

bool VASourceLeech::get_play_when_raytracing_completes() const
{
    return play_when_raytracing_completes;
}

void VASourceLeech::set_play_when_raytracing_completes(bool value)
{
    play_when_raytracing_completes = value;
}

float VASourceLeech::get_muffling_gain_lf() const
{
    return filter.get_gain();
}

float VASourceLeech::get_muffling_gain_hf() const
{
    return filter.get_gain_hf();
}

void VASourceLeech::_enter_tree()
{
    if (IS_EDITOR_HINT())
    {
        return;
    }

    emitter = Object::cast_to<va_godot::VAEmitter>(get_parent());

    if (!emitter)
    {
        VA_WARN(
            "'", get_name(),
            "' is a VASourceLeech but its parent is not a VAEmitter - "
            "it will never play. VASourceLeech must be a direct child of a "
            "VAEmitter (or VAListener) node whose own raytracing results it "
            "leeches off.");
        return;
    }

    va_world = va_godot::find_va_world(this);
}

void VASourceLeech::_exit_tree()
{
    emitter = nullptr;
    va_world = nullptr;
}

bool VASourceLeech::is_raytraced() const
{
    return emitter && emitter->is_raytraced();
}

bool VASourceLeech::play()
{
    if (!is_raytraced())
    {
        play_when_raytracing_completes = true;
        return false;
    }

    played = ALSource3D::play();

    return played;
}

void VASourceLeech::_process(double delta)
{
    ALSource3D::_process(delta);

    if (!is_raytraced())
    {
        return;
    }

    if (!played && play_when_raytracing_completes)
    {
        play();
    }

    if (!va_world)
    {
        return;
    }

    va_godot::VAEmitter *listener = va_world->get_listener();

    if (listener)
    {
        apply_raytracing_results(listener);
    }
}

// Same as VASource::apply_raytracing_results, but reads results off the
// parent VAEmitter this node leeches instead of an owned child emitter.
void VASourceLeech::apply_raytracing_results(va_godot::VAEmitter *other)
{
    effect = va_world->get_reverb_effect(emitter->get_handle());

    if (other->has_raytraced_target(emitter))
    {
        VALowPassFilter vaudio_filter = other->get_target_filter(emitter);
        update_filter(vaudio_filter.gainLF, vaudio_filter.gainHF, true);
    }
}
