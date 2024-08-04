#ifndef VIEW_H
#define VIEW_H

#include <standard.h>
#include <shaders.h>

void setupProjection(ShaderProgram* program);
void recalculateProjectionMatrix(ShaderProgram* program, int width, int height);
void setViewOffset(ShaderProgram* program, float x, float y, float z);
void setViewRotation(ShaderProgram* program, int x, int y, int z);

#endif