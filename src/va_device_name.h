#pragma once

// Shared by VAInputStreamSource's capture_device_name property; means "use the driver's default device" when set to this label rather
// than "" (an empty stored value would show as a second, unlabelled blank entry in the Inspector's enum dropdown alongside this label
// in hint_string). ALCaptureDevice::open still takes "" for "default" - the setter translates this label to "" only at that boundary.
static const char *DEFAULT_DEVICE_LABEL = "System Default";
