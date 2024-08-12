#ifndef VIEW_H
#define VIEW_H

#include <standard.h>
#include <rendering/shaders.h>

void setupProjection(ShaderProgram* program, float FOV);
void recalculateProjectionMatrix(ShaderProgram* program, int width, int height, float FOV);
void setViewOffset(ShaderProgram* program, float x, float y, float z);
void setViewRotation(ShaderProgram* program, int x, int y, int z);

#endif