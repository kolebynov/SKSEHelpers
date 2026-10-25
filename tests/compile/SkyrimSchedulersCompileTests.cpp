#include "SkyrimSchedulers.h"

// Instantiates both HookInstaller handlers; never called.
void InstallSkyrimSchedulers() { [[maybe_unused]] const auto schedulers = SkyrimSchedulers::Install(); }
