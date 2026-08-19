#pragma once

// Shared by VAOpenALSettings and VAInputStreamSource's device_name/
// capture_device_name properties. Both mean "use the driver's default
// device" when set to this label rather than "" - see those classes' header
// doc comments for why "" itself isn't used as the stored/displayed value
// (the Inspector's enum dropdown always shows the property's current value
// as one of its entries, so an empty stored value would show up as a
// second, unlabelled blank entry alongside this label in hint_string).
// ALManager::reinitialize and ALCaptureDevice::open still take "" for
// "default" - each class's setter translates DEFAULT_DEVICE_LABEL to "" only
// at that call boundary.
static const char *DEFAULT_DEVICE_LABEL = "(Default)";
