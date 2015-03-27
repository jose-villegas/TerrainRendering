#include "Commons.h"
#include "TransformationMatrices.h"

bool TransformationMatrices::modelMatrixChanged = false;
bool TransformationMatrices::projectionMatrixChanged = false;
bool TransformationMatrices::viewMatrixChanged = false;

glm::mat4 TransformationMatrices::model = glm::mat4(1);
glm::mat4 TransformationMatrices::view = glm::mat4(1);
glm::mat4 TransformationMatrices::projection = glm::mat4(1);
glm::mat4 TransformationMatrices::normal = glm::mat4(1);
glm::mat4 TransformationMatrices::modelView = glm::mat4(1);
glm::mat4 TransformationMatrices::modelViewProjection = glm::mat4(1);

const glm::mat4 & TransformationMatrices::ModelViewProjection()
{
    if(!(modelMatrixChanged || viewMatrixChanged) && projectionMatrixChanged)
    {
        modelViewProjection = projection * modelView;
        projectionMatrixChanged = false;
        modelMatrixChanged = viewMatrixChanged = projectionMatrixChanged = false;
    }

    if(modelMatrixChanged || viewMatrixChanged)
    {
        modelView = view * model;
        modelViewProjection = projection * modelView;
        normal = glm::transpose(glm::inverse(modelView));
        modelMatrixChanged = viewMatrixChanged = projectionMatrixChanged = false;
    }

    return modelViewProjection;
}

const glm::mat4 & TransformationMatrices::ModelView()
{
    if(modelMatrixChanged || viewMatrixChanged)
    {
        modelView = view * model;
        normal = glm::transpose(glm::inverse(modelView));
        modelMatrixChanged = viewMatrixChanged = false;
    }

    return modelView;
}

const glm::mat4 & TransformationMatrices::Normal()
{
    if(modelMatrixChanged || viewMatrixChanged)
    {
        modelView = view * model;
        normal = glm::transpose(glm::inverse(modelView));
        modelMatrixChanged = viewMatrixChanged = false;
    }

    return normal;
}

void TransformationMatrices::Projection(const glm::mat4 &matrix)
{
    if(matrix != projection)
    {
        projectionMatrixChanged = true;
        projection = matrix;
    }
    else
    {
        projectionMatrixChanged = false;
    }
}

void TransformationMatrices::View(const glm::mat4 &matrix)
{
    if(matrix != view)
    {
        viewMatrixChanged = true;
        view = matrix;
    }
    else
    {
        viewMatrixChanged = false;
    }
}

void TransformationMatrices::Model(const glm::mat4 &matrix)
{
    if(matrix != model)
    {
        modelMatrixChanged = true;
        model = matrix;
    }
    else
    {
        modelMatrixChanged = false;
    }
}

TransformationMatrices::TransformationMatrices()
{
}


TransformationMatrices::~TransformationMatrices()
{
}
