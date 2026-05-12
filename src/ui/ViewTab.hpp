#pragma once

#include <QString>
#include <QWidget>
#include <vector>

#include "Mesh.hpp"

QT_BEGIN_NAMESPACE
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QSpinBox;
QT_END_NAMESPACE

namespace qi::storage {
class ExperimentRepository;
}

namespace qi::ui {

class Viewport3D;

// Right pane: 3D viewport. Left pane: list of saved experiments.
// On selection — recomputes meshes (re-triangulates each surface from its
// stored config) and polylines (re-intersects each pair), then pushes them
// to the viewport. Mesh data is intentionally NOT stored in the DB; only
// numeric metadata (triangle count, timing) is persisted.
class ViewTab : public QWidget {
    Q_OBJECT
public:
    explicit ViewTab(QWidget* parent = nullptr);

    void setRepository(qi::storage::ExperimentRepository* repo);

    int experimentListSize() const;
    int currentMeshCount() const;
    int currentPolylineCount() const;

public slots:
    void refreshExperimentList();
    void loadExperiment(int id);

private slots:
    void onExperimentSelected();
    void onBvhSurfaceChanged(int index);
    void onBvhDepthChanged(int value);

private:
    void rebuildBvhForSelectedSurface();

    QListWidget* experimentList_;
    Viewport3D* viewport_;
    QComboBox* bvhSurfaceCombo_;
    QSpinBox* bvhDepthSpin_;
    qi::storage::ExperimentRepository* repo_ = nullptr;
    int loadedMeshCount_ = 0;
    int loadedPolylineCount_ = 0;
    // Meshes from the currently displayed experiment, kept so the BVH can be
    // (re)built on demand when the user picks a surface from the BVH combo.
    std::vector<qi::mesh::Mesh> currentMeshes_;
};

}  // namespace qi::ui
