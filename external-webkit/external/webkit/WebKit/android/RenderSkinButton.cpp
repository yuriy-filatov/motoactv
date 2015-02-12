/*
 * Copyright 2006, The Android Open Source Project
 * Copyright (C) 2009-2010 Motorola, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define LOG_TAG "WebCore"

#include "config.h"
#include "CString.h"
#include "android_graphics.h"
#include "Document.h"
#include "IntRect.h"
#include "Node.h"
#include "RenderSkinButton.h"
#include "SkCanvas.h"
#include "SkNinePatch.h"
#include "SkRect.h"
#include <utils/Debug.h>
#include <utils/Log.h>

struct PatchData {
    const char* name;
    int8_t outset, margin;
};

static const PatchData gFiles[] =
    {
        // BEGIN Motorola dtw768 2010.07.15 IKSTABLEONE-1652: PT:MB810:Droid2:keys within the browser are still in dark gray on Droid2
        { "btn_default_normal_disable_browser_ui.9.png", 2, 7 },
        { "btn_default_normal_browser_ui.9.png", 2, 7 },
        { "btn_default_selected_browser_ui.9.png", 2, 7 },
        { "btn_default_pressed_browser_ui.9.png", 2, 7 }
        // END IKSTABLEONE-1652
    };

static SkBitmap gButton[sizeof(gFiles)/sizeof(gFiles[0])];
static bool gDecoded;
static bool gHighRes;

// BEGIN Motorola e7391c 2010.03.18 IKMAP-7320:Download and Preview icons in gmail appear white.
static bool gQVGA;
// END IKMAP-7320


namespace WebCore {

void RenderSkinButton::Init(android::AssetManager* am, String drawableDirectory)
{
    static bool gInited;
    if (gInited)
        return;

    gInited = true;
    gDecoded = true;
    gHighRes = drawableDirectory[drawableDirectory.length() - 5] == 'h';
    // BEGIN Motorola e7391c 2010.03.18 IKMAP-7320:Download and Preview icons in gmail appear white.
    gQVGA = drawableDirectory[drawableDirectory.length() - 5] == 'l';
    // END IKMAP-7320
    for (size_t i = 0; i < sizeof(gFiles)/sizeof(gFiles[0]); i++) {
        // BEGIN Motorola e7391c 2010.03.18 IKMAP-7320:Download and Preview icons in gmail appear white.
        String path;
        if (gQVGA) {
            path = drawableDirectory + "zz_moto_webkit_" + gFiles[i].name;
        }
        else {
            path = drawableDirectory + gFiles[i].name;
        }
        // END IKMAP-7320
        if (!RenderSkinAndroid::DecodeBitmap(am, path.utf8().data(), &gButton[i])) {
            gDecoded = false;
            LOGD("RenderSkinButton::Init: button assets failed to decode\n\tBrowser buttons will not draw");
            break;
        }
    }

    // Ensure our enums properly line up with our arrays.
    android::CompileTimeAssert<(RenderSkinAndroid::kDisabled == 0)>     a1;
    android::CompileTimeAssert<(RenderSkinAndroid::kNormal == 1)>       a2;
    android::CompileTimeAssert<(RenderSkinAndroid::kFocused == 2)>      a3;
    android::CompileTimeAssert<(RenderSkinAndroid::kPressed == 3)>      a4;
}

void RenderSkinButton::Draw(SkCanvas* canvas, const IntRect& r, RenderSkinAndroid::State newState)
{
    // If we failed to decode, do nothing.  This way the browser still works,
    // and webkit will still draw the label and layout space for us.
    if (!gDecoded) {
        return;
    }
    
    // Ensure that the state is within the valid range of our array.
    SkASSERT(static_cast<unsigned>(newState) < 
            static_cast<unsigned>(RenderSkinAndroid::kNumStates));

    // Set up the ninepatch information for drawing.
    SkRect bounds(r);
    const PatchData& pd = gFiles[newState];
    int marginValue = pd.margin + pd.outset;

    SkIRect margin;

    margin.set(marginValue, marginValue, marginValue, marginValue);
    if (gHighRes) {
    /* FIXME: it shoudn't be necessary to offset the button here,
       but gives the right results. */
        bounds.offset(0, SK_Scalar1 * 2);
    /* FIXME: This temporarily gets around the fact that the margin values and
       positioning were created for a low res asset, which was used on
       g1-like devices. A better fix would be to read the offset information
       out of the png. */
        margin.set(10, 9, 10, 14);
    }
    // Draw to the canvas.
    SkNinePatch::DrawNine(canvas, bounds, gButton[newState], margin);
}

} //WebCore
