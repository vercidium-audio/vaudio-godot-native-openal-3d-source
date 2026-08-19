#pragma once

// Shared by VAInputStreamSource's capture_device_name property. Means "use
// the driver's default device" when set to this label rather than "" - see
// that class's header doc comment for why "" itself isn't used as the
// stored/displayed value (the Inspector's enum dropdown always shows the
// property's current value as one of its entries, so an empty stored value
// would show up as a second, unlabelled blank entry alongside this label in
// hint_string). ALCaptureDevice::open still takes "" for "default" - the
// class's setter translates DEFAULT_DEVICE_LABEL to "" only at that call
// boundary.
static const char *DEFAULT_DEVICE_LABEL = "System Default";
