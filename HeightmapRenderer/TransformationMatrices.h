#pragma once

class TransformationMatrices
{
    private:
        static bool projectionMatrixChanged, viewMatrixChanged, modelMatrixChanged;

        static glm::mat4 model;
        static glm::mat4 view;
        static glm::mat4 projection;
        static glm::mat4 normal;
        static glm::mat4 modelView;
        static glm::mat4 modelViewProjection;
        static glm::mat4 viewProjection;
    public:
        static const glm::mat4 &Projection() { return projection; };
        static const glm::mat4 &View() { return view; };
        static const glm::mat4 &Model() { return model; };

        static const glm::mat4 &Normal();
        static const glm::mat4 &ModelView();
        static const glm::mat4 &ModelViewProjection();

        static void Projection(const glm::mat4 &matrix);
        static void View(const glm::mat4 &matrix);
        static void Model(const glm::mat4 &matrix);
    private:
        TransformationMatrices();
        ~TransformationMatrices();
};

