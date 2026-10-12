#ifndef PATHPREVIEW_H
#define PATHPREVIEW_H
#include <QWidget>
#include "DijkstraSimulator.h"

// A passive tooltip: inspecting a route never changes the simulation target.
class PathPreview : public QWidget {
public:
    explicit PathPreview(const DijkstraSimulator *simulator, QWidget *parent=nullptr);
    void setVertex(int vertex);
    int vertex() const { return destination; }
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    const DijkstraSimulator *sim;
    int destination=-1;
};
#endif
