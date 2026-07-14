#pragma once

#include "raylib.h"
#include "ui_widget.hpp"

struct ImageFitResult
{
    Rectangle src;
    Rectangle dst;
};

inline ImageFitResult computeImageFit(
    float texW, float texH,
    float dstX, float dstY, float dstW, float dstH,
    ImageFit fit)
{
    float srcX = 0, srcY = 0, srcW = texW, srcH = texH;
    float outX = dstX, outY = dstY, outW = dstW, outH = dstH;

    if (fit == ImageFit::Contain)
    {
        float scale = (dstW / texW < dstH / texH) ? dstW / texW : dstH / texH;
        outW = texW * scale;
        outH = texH * scale;
        outX = dstX + (dstW - outW) / 2.0f;
        outY = dstY + (dstH - outH) / 2.0f;
    }
    else if (fit == ImageFit::Cover)
    {
        float scale = (dstW / texW > dstH / texH) ? dstW / texW : dstH / texH;
        srcX = texW / 2.0f - dstW / (2.0f * scale);
        srcY = texH / 2.0f - dstH / (2.0f * scale);
        srcW = dstW / scale;
        srcH = dstH / scale;
    }

    return {Rectangle{srcX, srcY, srcW, srcH}, Rectangle{outX, outY, outW, outH}};
}
