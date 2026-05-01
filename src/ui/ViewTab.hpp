#pragma once

#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
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

private:
    QListWidget* experimentList_;
    Viewport3D* viewport_;
    qi::storage::ExperimentRepository* repo_ = nullptr;
    int loadedMeshCount_ = 0;
    int loadedPolylineCount_ = 0;
};

}  // namespace qi::ui
