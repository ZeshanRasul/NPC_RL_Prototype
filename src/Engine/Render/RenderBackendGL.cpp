#include "RenderBackendGL.h"

RenderBackend* RenderBackendGL::CreateRenderBackend() {
	return new RenderBackendGL();
}