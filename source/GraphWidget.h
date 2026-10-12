#ifndef GRAPHWIDGET_H
#define GRAPHWIDGET_H
#include <QWidget>
#include <QVariantAnimation>
#include <set>
#include <QLineF>
#include <QPainterPath>
#include "DijkstraSimulator.h"
class QLineEdit;
class QSpinBox;
class GraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit GraphWidget(DijkstraSimulator *simulator, QWidget *parent = nullptr);
    void animateStep(int duration, bool reset = false);
    void resetLayout();
    bool removeVertex(int id);
    bool isEditing() const;
    void setInstantResult(bool enabled);
    void zoomBy(qreal factor);
    void resetView();
    qreal zoomFactor() const { return zoom; }
signals:
    void zoomChanged(qreal factor);
    void graphEdited();
    void interactionStarted();
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
private:
    struct EdgeHit { QRectF box; int from; QPainterPath path; int id; };
    std::vector<QPointF> positions;
    std::vector<EdgeHit> edgeHits;
    int draggedNode = -1;
    int editingNode = -1;
    QLineEdit *nameEditor;
    QSpinBox *weightEditor;
    int editingFrom = -1;
    int editingEdgeId = -1;
    void finishWeightEdit(bool cancel = false);
    void positionWeightEditor();
    std::set<int> selectedNodes;
    std::set<int> selectedEdges;
    std::set<int> selectionBaseNodes;
    std::set<int> selectionBaseEdges;
    bool selecting = false, panning = false;
    QPointF selectionStart, selectionEnd, panStart;
    qreal zoom = 1.0;
    QPointF viewOffset;
    QPointF toWorld(QPointF screen) const { return (screen-viewOffset)/zoom; }
    QPointF toScreen(QPointF world) const { return world*zoom+viewOffset; }
    void zoomAt(qreal factor, QPointF anchor);
    int edgeAt(QPointF point) const;
    bool instantResult = false;
    void finishNameEdit(bool cancel = false);
    void positionNameEditor();
    QPointF dragOffset;
    int connectingNode = -1;
    Qt::MouseButton connectionButton = Qt::RightButton;
    QPointF connectionStart;
    QPointF connectionCursor;
    bool connectionDragged = false;
    bool suppressMouseContext = false;
    void beginConnection(int node, Qt::MouseButton button, QPointF worldPosition);
    void ensurePositions();
    int nodeAt(QPointF point) const;
    DijkstraSimulator *sim;
    QVariantAnimation *animation;
    qreal progress = 1.0;
    std::vector<int> previousDistances;
};
#endif
