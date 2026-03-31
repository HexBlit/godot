/**************************************************************************/
/*  libgodot_web.cpp                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "os_web.h"

#include "core/extension/godot_instance.h"
#include "core/extension/libgodot.h"
#include "core/io/resource_loader.h"
#include "main/main.h"

#include <emscripten.h>

static OS_Web *os = nullptr;
static GodotInstance *instance = nullptr;

extern "C" {

static void minimal_init(void *userdata, GDExtensionInitializationLevel p_level) {
	// No-op: we don't register any classes yet
}

static void minimal_deinit(void *userdata, GDExtensionInitializationLevel p_level) {
	// No-op
}

EMSCRIPTEN_KEEPALIVE
GDExtensionBool minimal_gdextension_init(
	GDExtensionInterfaceGetProcAddress p_get_proc_address,
	GDExtensionClassLibraryPtr p_library,
	GDExtensionInitialization *r_initialization) {

	r_initialization->minimum_initialization_level = GDEXTENSION_INITIALIZATION_CORE;
	r_initialization->initialize = minimal_init;
	r_initialization->deinitialize = minimal_deinit;
	r_initialization->userdata = nullptr;

	return true;
}

EMSCRIPTEN_KEEPALIVE
GDExtensionObjectPtr libgodot_create_godot_instance(int p_argc, char *p_argv[], GDExtensionInitializationFunction p_init_func) {
	ERR_FAIL_COND_V_MSG(instance != nullptr, nullptr, "Only one Godot Instance may be created.");

	os = new OS_Web();

	Error err = Main::setup(p_argv[0], p_argc - 1, &p_argv[1], false);
	if (err != OK) {
		return nullptr;
	}

	// Match web_main.cpp behavior for web compatibility.
	ResourceLoader::set_abort_on_missing_resources(false);

	instance = memnew(GodotInstance);
	if (!instance->initialize(p_init_func)) {
		memdelete(instance);
		return nullptr;
	}

	return (GDExtensionObjectPtr)instance;
}

EMSCRIPTEN_KEEPALIVE
int libgodot_web_start() {
	ERR_FAIL_COND_V_MSG(instance == nullptr, 0, "No Godot instance created.");
	return instance->start() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int libgodot_web_iteration() {
	ERR_FAIL_COND_V_MSG(instance == nullptr, 1, "No Godot instance created.");
	// Main::iteration() returns true when the engine wants to exit.
	// We return 0 to continue, 1 to quit.
	return instance->iteration() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
void libgodot_web_stop() {
	if (instance != nullptr) {
		instance->stop();
	}
}

EMSCRIPTEN_KEEPALIVE
void libgodot_destroy_godot_instance(GDExtensionObjectPtr p_godot_instance) {
	GodotInstance *godot_instance = (GodotInstance *)p_godot_instance;
	if (instance == godot_instance) {
		godot_instance->stop();
		memdelete(godot_instance);
		instance = nullptr;
		Main::cleanup();
	}
}

} // extern "C"