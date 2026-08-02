# Overview

This repo is a native Godot GDExtension (C++) plugin wrapping the vaudio C SDK, with OpenAL
Soft as the audio backend, for non-Mono ("Native Godot") projects.

This plugin links against the package vaudionative C DLL, not the source.

# How to use

To build, run build.bat.

# C++ code style

- Allman brace style: opening curly braces go on their own new line (for functions, classes,
  control flow, everything), not K&R/"same line" style.
- 4 spaces per indent level, not tabs.

# References

See user_context.md for more info (may not exist, this file is git-ignored)