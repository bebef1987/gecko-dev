/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "WebGLExtensions.h"

#include "GLContext.h"
#include "mozilla/dom/WebGLRenderingContextBinding.h"
#include "WebGLContext.h"

using namespace mozilla;

WebGLExtensionColorBufferHalfFloat::WebGLExtensionColorBufferHalfFloat(WebGLContext* context)
    : WebGLExtensionBase(context)
{
    MOZ_ASSERT(IsSupported(context));
}

WebGLExtensionColorBufferHalfFloat::~WebGLExtensionColorBufferHalfFloat()
{
}

bool
WebGLExtensionColorBufferHalfFloat::IsSupported(const WebGLContext* context)
{
    return context->GL()->IsSupported(gl::GLFeature::renderbuffer_color_half_float) &&
           context->GL()->IsSupported(gl::GLFeature::frag_color_float);
}

IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionColorBufferHalfFloat)
