#include "GraphWidget.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QResizeEvent>
#include <QMenu>
#include <QWidgetAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QLabel>
#include <QApplication>
#include <QLineEdit>
#include <QKeyEvent>
#include <QToolTip>
#include <QValidator>
#include <QTextBoundaryFinder>
#include <QFontMetricsF>
#include <QInputMethodEvent>
#include <algorithm>
#include <cmath>
#include <map>
#include <QPainterPath>
#include <QPainterPathStroker>
namespace {
constexpr int NameWidth = 36;
constexpr int NameHeight = 24;
QFont vertexNameFont() {
    QFont font("Segoe UI", 12, QFont::DemiBold);
    font.setPixelSize(16);
    return font;
}
class VertexNameValidator : public QValidator {
public:
    explicit VertexNameValidator(QLineEdit *editor) : QValidator(editor), editor(editor) {}
    State validate(QString &input, int &position) const override {
        Q_UNUSED(position);
        QTextBoundaryFinder boundaries(QTextBoundaryFinder::Grapheme, input);
        int count = 0;
        while (boundaries.toNextBoundary() != -1) ++count;
        const QFontMetricsF metrics(editor->font());
        const QRectF ink = metrics.boundingRect(input);
        // Reserve room for the caret as well: reject the keystroke before the
        // line edit can scroll text beyond the circle's inner name area.
        if (count > 4 || std::max(metrics.horizontalAdvance(input), ink.width()) + 2 > NameWidth
            || ink.height() > NameHeight) return Invalid;
        return input.trimmed().isEmpty() ? Intermediate : Acceptable;
    }
private:
    QLineEdit *editor;
};
class VertexNameEditor : public QLineEdit {
public:
    explicit VertexNameEditor(QWidget *parent) : QLineEdit(parent) {}
protected:
    void inputMethodEvent(QInputMethodEvent *event) override {
        // QLineEdit's validator only checks committed text. Vietnamese input
        // methods can otherwise keep adding preedit text and scroll the field.
        QString proposed = text();
        int insertion = cursorPosition();
        if (hasSelectedText()) {
            insertion = selectionStart();
            proposed.remove(insertion, selectedText().size());
        }
        insertion = std::clamp(insertion + event->replacementStart(), 0, int(proposed.size()));
        proposed.remove(insertion, event->replacementLength());
        proposed.insert(insertion, event->commitString() + event->preeditString());
        int cursor = insertion;
        if (validator() && validator()->validate(proposed, cursor) == QValidator::Invalid) {
            event->accept();
            return;
        }
        QLineEdit::inputMethodEvent(event);
    }
};
}
GraphWidget::GraphWidget(DijkstraSimulator *simulator, QWidget *parent) : QWidget(parent), sim(simulator) {
    setMinimumSize(400,260); setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    nameEditor = new VertexNameEditor(this);
    nameEditor->setObjectName("inlineVertexName");
    nameEditor->setAlignment(Qt::AlignCenter);
    nameEditor->setFrame(false);
    nameEditor->setFont(vertexNameFont());
    nameEditor->setValidator(new VertexNameValidator(nameEditor));
    nameEditor->setToolTip("Tối đa 4 ký tự; tên phải nằm gọn trong vòng tròn.");
    nameEditor->setStyleSheet("QLineEdit { background: transparent; border: none; padding: 0; font-family: 'Segoe UI'; font-size: 16px; font-weight: 600; color: #334155; selection-background-color: #fecaca; selection-color: #991b1b; }");
    nameEditor->hide();
    nameEditor->installEventFilter(this);
    connect(nameEditor, &QLineEdit::textChanged, this, [this] { positionNameEditor(); update(); });
    weightEditor = new QSpinBox(this);
    weightEditor->setObjectName("inlineEdgeWeight");
    weightEditor->setRange(0, DijkstraSimulator::MaxEdgeWeight);
    weightEditor->setButtonSymbols(QAbstractSpinBox::NoButtons);
    weightEditor->setKeyboardTracking(false);
    weightEditor->setAlignment(Qt::AlignCenter);
    weightEditor->setFrame(false);
    weightEditor->setFont(QFont("Segoe UI",10,QFont::DemiBold));
    weightEditor->setStyleSheet("QSpinBox, QSpinBox QLineEdit { background: transparent; border: none; padding: 0; color: #475569; selection-background-color: #fecaca; selection-color: #991b1b; }");
    weightEditor->hide();
    weightEditor->installEventFilter(this);
    weightEditor->findChild<QLineEdit*>()->installEventFilter(this);
    setToolTip("Lăn chuột: zoom · Kéo chuột giữa: di chuyển khung nhìn · Kéo vùng trắng: chọn nhiều · Ctrl: thêm/bớt chọn");
    animation = new QVariantAnimation(this);
    animation->setStartValue(0.0);
    animation->setEndValue(1.0);
    animation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(animation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        progress = value.toReal();
        update();
    });
}
void GraphWidget::animateStep(int duration, bool reset) {
    animation->stop();
    if (reset) {
        previousDistances = sim->dist;
        progress = 1.0;
        update();
        return;
    }
    animation->setDuration(std::max(80, duration));
    animation->start();
}
void GraphWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.setRenderHint(QPainter::TextAntialiasing);
    p.fillRect(rect(), QColor("#f1f5f9"));
    p.setPen(QPen(QColor("#dbe3ef"),1)); p.setBrush(Qt::white); p.drawRoundedRect(QRectF(rect()).adjusted(0.5,0.5,-0.5,-0.5),10,10);
    QPainterPath canvasClip; canvasClip.addRoundedRect(QRectF(rect()).adjusted(1,1,-1,-1),9,9);
    p.setClipPath(canvasClip);
    p.setPen(QColor("#e2e8f0"));
    for(int x=22;x<width()-12;x+=24) for(int y=22;y<height()-12;y+=24) p.drawPoint(x,y);
    if(sim->graph.empty()) { edgeHits.clear(); p.setPen(QColor("#64748b")); p.drawText(rect(), Qt::AlignCenter, "Bấm chuột phải để thêm đỉnh"); return; }
    ensurePositions();
    p.translate(viewOffset); p.scale(zoom, zoom);
    edgeHits.clear();
    auto point = [&](const Node &n) { return positions[n.id]; };
    struct WeightLabel { QPointF center; int weight; QColor color; int from; QPainterPath path; int id; };
    std::vector<WeightLabel> labels;
    std::map<std::pair<int,int>,std::set<int>> parallelEdges;
    size_t visibleEdges=0;
    for(const auto &node:sim->graph) for(const auto &edge:node.edges) {
        parallelEdges[std::minmax(node.id,edge.target)].insert(edge.id);
        if(edge.directed || node.id<edge.target) ++visibleEdges;
    }
    labels.reserve(visibleEdges);
    edgeHits.reserve(visibleEdges);
    std::set<int> shortestEdgeIds;
    for(const auto &[from,to]:sim->shortestPathEdges) {
        Q_UNUSED(from);
        shortestEdgeIds.insert(sim->prevEdgeIds[to]);
    }
    int examinedEdgeId=-1;
    if(!instantResult && sim->currentNode>=0 && sim->currentExaminedTarget>=0 &&
       sim->currentEdgeIndex>=0 && sim->currentEdgeIndex<static_cast<int>(sim->graph[sim->currentNode].edges.size()))
        examinedEdgeId=sim->graph[sim->currentNode].edges[sim->currentEdgeIndex].id;
    for(const auto &n:sim->graph) for(const auto &e:n.edges) {
        if(!e.directed && n.id>e.target) continue;
        auto a=point(n),b=point(sim->graph[e.target]);
        const auto delta0=b-a;
        const qreal len0=std::hypot(delta0.x(),delta0.y());
        const auto &siblings=parallelEdges.at(std::minmax(n.id,e.target));
        QPainterPath path;
        QPointF arrowUnit;
        if(siblings.size()>1 && len0>0) {
            const qreal lane=(static_cast<qreal>(siblings.size())-1)/2-std::distance(siblings.begin(),siblings.find(e.id));
            const qreal offsetSize=lane*24*(n.id<e.target?1:-1);
            const QPointF offset(-delta0.y()/len0*offsetSize,delta0.x()/len0*offsetSize);
            if(siblings.size()>=3) {
                // Fan out from the node boundary rather than translating endpoints
                // outside the nodes when there are many parallel edges.
                const QPointF control=(a+b)/2+offset*2;
                const QLineF startTangent(a,control), endTangent(control,b);
                const QPointF startUnit=(control-a)/startTangent.length();
                arrowUnit=(b-control)/endTangent.length();
                a+=startUnit*23; b-=arrowUnit*(e.directed?29:23);
                path.moveTo(a); path.quadTo(control,b);
            } else { a+=offset; b+=offset; }
        }
        if(path.isEmpty()) {path.moveTo(a);path.lineTo(b);arrowUnit=len0>0?delta0/len0:QPointF(1,0);}
        QColor color("#cbd5e1"); qreal lineWidth=1.8;
        const bool shortest=shortestEdgeIds.contains(e.id);
        const bool examining=e.id==examinedEdgeId;
        if(shortest) {color=QColor("#db2777");lineWidth=4;} else if(examining) {color=QColor("#d97706");lineWidth=3;} else if(!instantResult && (sim->prevEdgeIds[e.target]==e.id||(!e.directed&&sim->prevEdgeIds[n.id]==e.id))) {color=QColor("#64748b");lineWidth=2.4;}
        if(selectedEdges.contains(e.id)) {color=QColor("#ef4444"); lineWidth=4;}
        if(e.directed && len0>0) {
            const QPointF unit=arrowUnit, normal(-unit.y(),unit.x());
            const QPointF tip=siblings.size()>=3?b:b-unit*29;
            p.setPen(Qt::NoPen); p.setBrush(color);
            p.drawPolygon(QPolygonF{tip,tip-unit*12+normal*6,tip-unit*12-normal*6});
        }
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(color,lineWidth,examining && !shortest ? Qt::DashLine : Qt::SolidLine,Qt::RoundCap)); p.drawPath(path);
        if (examining && progress < 1.0) {
            const bool forward=sim->currentNode==n.id;
            QPainterPath trail(path.pointAtPercent(forward?0:1));
            for(int i=1;i<=40;++i) trail.lineTo(path.pointAtPercent(forward?progress*i/40:1-progress*i/40));
            const QPointF head=trail.currentPosition();
            QColor glow("#f59e0b"); glow.setAlpha(55);
            p.setPen(QPen(glow, 12, Qt::SolidLine, Qt::RoundCap)); p.drawPath(trail);
            p.setPen(QPen(QColor("#f59e0b"), 4, Qt::SolidLine, Qt::RoundCap)); p.drawPath(trail);
            p.setPen(Qt::NoPen); p.setBrush(QColor("#f59e0b")); p.drawEllipse(head, 6, 6);
        }
        if (shortest && progress < 1.0) {
            p.setBrush(Qt::NoBrush);
            QColor glow("#ec4899"); glow.setAlpha(static_cast<int>(80 * (1 - progress)));
            p.setPen(QPen(glow, 14, Qt::SolidLine, Qt::RoundCap)); p.drawPath(path);
        }
        // Separate labels at crossing edges while keeping them close to their own edge.
        QPointF mid=path.pointAtPercent(0.5); const qreal length=path.length();
        for(int attempt=0;attempt<4;++attempt) { const bool collision=std::any_of(labels.begin(),labels.end(),[&](const auto &label){ return std::abs(mid.x()-label.center.x())<40 && std::abs(mid.y()-label.center.y())<28; }); if(!collision) break; mid=path.pointAtPercent(0.5+(attempt%2==0?1:-1)*std::min((attempt/2+1)*24.0/std::max(1.0,length),0.22)); }
        labels.push_back({mid,e.weight,color,n.id,path,e.id});
    }
    p.setFont(QFont("Segoe UI",10,QFont::DemiBold));
    for(const auto &label:labels) { const QString value=QString::number(label.weight); const qreal boxWidth=std::max(34,p.fontMetrics().horizontalAdvance(value)+12); QRectF box(label.center.x()-boxWidth/2,label.center.y()-12,boxWidth,24);
        edgeHits.push_back({box, label.from, label.path, label.id}); p.setPen(QPen(label.color,1));p.setBrush(Qt::white);p.drawRoundedRect(box,5,5);p.setPen(QColor("#475569"));if(label.id!=editingEdgeId) p.drawText(box,Qt::AlignCenter,value); }
    for(const auto &n:sim->graph) {
        const QPointF c=point(n); QColor border("#94a3b8"),fill("#ffffff"),text("#334155");
        if(sim->visited[n.id]) {border=QColor("#0f766e");fill=QColor("#ccfbf1");}
        if(n.id==sim->targetNode) border=QColor("#7c3aed");
        if(!instantResult && n.id==sim->currentExaminedTarget) {border=QColor("#d97706");fill=QColor("#fef3c7");}
        if(!instantResult && n.id==sim->currentNode) {border=QColor("#2563eb");fill=border;text=Qt::white;}
        if(n.id==editingNode) { fill=Qt::white; text=QColor("#334155"); }
        const bool changed = previousDistances.size() == sim->dist.size() && previousDistances[n.id] != sim->dist[n.id];
        if (progress < 1.0 && (n.id == sim->currentNode || n.id == sim->currentExaminedTarget || changed)) {
            QColor halo = border; halo.setAlpha(static_cast<int>(100 * (1 - progress)));
            p.setPen(QPen(halo, 5)); p.setBrush(Qt::NoBrush);
            p.drawEllipse(c, 27 + 14 * progress, 27 + 14 * progress);
        }
        if(n.id==sim->startNode||n.id==sim->targetNode) {p.setPen(QPen(QColor(n.id==sim->targetNode ? "#7c3aed" : "#2563eb"),2));p.setBrush(Qt::NoBrush);p.drawEllipse(c,28,28);}
        if(selectedNodes.contains(n.id)) {
            p.setPen(QPen(QColor("#ef4444"), 3)); p.setBrush(Qt::NoBrush); p.drawEllipse(c,33,33);
        }
        p.setBrush(fill);p.setPen(QPen(border,2));p.drawEllipse(c,23,23);p.setFont(QFont("Segoe UI",12,QFont::DemiBold));p.setPen(text);
        if(n.id!=editingNode) {
            const QString name=QString::fromStdString(n.name);
            QFont font = vertexNameFont();
            // Imported names may be longer: fit their display without changing the data.
            while (font.pixelSize() > 1) {
                const QFontMetricsF metrics(font);
                if (std::max(metrics.horizontalAdvance(name), metrics.boundingRect(name).width()) <= NameWidth
                    && metrics.boundingRect(name).height() <= NameHeight) break;
                font.setPixelSize(font.pixelSize()-1);
            }
            p.setFont(font);
            p.save();
            const QRectF nameBox(c.x()-NameWidth/2,c.y()-NameHeight/2,NameWidth,NameHeight);
            p.setClipRect(nameBox);
            p.drawText(nameBox,Qt::AlignCenter,name);
            p.restore();
        }
        if(sim->visited[n.id]) {
            const QRectF badge(c.x()+13,c.y()-29,18,18);
            p.setPen(QPen(Qt::white,2)); p.setBrush(QColor("#0f766e")); p.drawEllipse(badge);
            p.setPen(QPen(Qt::white,2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
            p.drawLine(badge.topLeft()+QPointF(5,9),badge.topLeft()+QPointF(8,12));
            p.drawLine(badge.topLeft()+QPointF(8,12),badge.topLeft()+QPointF(13,6));
        }
        p.setFont(QFont("Segoe UI",9));p.setPen(QColor("#64748b")); const QString distance=sim->dist[n.id]==std::numeric_limits<int>::max()?QString::fromUtf8("∞"):QString::number(sim->dist[n.id]);
        p.drawText(QRectF(c.x()-48,c.y()+31,96,20),Qt::AlignCenter,"d = "+distance);
        if(n.id==sim->startNode||n.id==sim->targetNode) p.drawText(QRectF(c.x()-50,c.y()-53,100,20),Qt::AlignCenter,n.id==sim->startNode?"Xuất phát":"Đích đến");
    }
    if (connectingNode >= 0 && connectionDragged) {
        const int target = nodeAt(connectionCursor);
        const bool valid = target >= 0 && target != connectingNode;
        const QColor color(valid ? "#0f766e" : "#2563eb");
        p.setPen(QPen(color, 3, Qt::DashLine, Qt::RoundCap));
        p.drawLine(positions[connectingNode], valid ? positions[target] : connectionCursor);
        p.setBrush(Qt::NoBrush);
        if (valid) p.drawEllipse(positions[target], 31, 31);
        else p.drawEllipse(connectionCursor, 5, 5);
    }
    if(selecting) {
        p.setPen(QPen(QColor("#2563eb"),1/zoom)); p.setBrush(QColor(59,130,246,35));
        p.drawRect(QRectF(selectionStart,selectionEnd).normalized());
    }
    if (progress >= 1.0) previousDistances = sim->dist;
}



void GraphWidget::resetLayout() {
    finishNameEdit(true); finishWeightEdit(true);
    selectedNodes.clear(); selectedEdges.clear(); selecting=panning=false; zoom=1; viewOffset={};
    emit zoomChanged(zoom);
    positions.clear(); edgeHits.clear(); draggedNode = connectingNode = -1;
    connectionDragged = suppressMouseContext = false; unsetCursor();
    update();
}

void GraphWidget::ensurePositions() {
    if (positions.size() == sim->graph.size()) return;
    positions.clear();
    if (sim->graph.empty()) return;
    int minX=sim->graph.front().x, maxX=minX, minY=sim->graph.front().y, maxY=minY;
    for (const auto &n : sim->graph) {
        minX=std::min(minX,n.x); maxX=std::max(maxX,n.x);
        minY=std::min(minY,n.y); maxY=std::max(maxY,n.y);
    }
    const qreal scale=std::min((width()-130.0)/std::max(1,maxX-minX),(height()-136.0)/std::max(1,maxY-minY));
    for (const auto &n : sim->graph)
        positions.push_back(QPointF(width()/2.0+(n.x-(minX+maxX)/2.0)*scale,
                                   height()/2.0+(n.y-(minY+maxY)/2.0)*scale));
}

int GraphWidget::nodeAt(QPointF point) const {
    for (int i=static_cast<int>(positions.size())-1; i>=0; --i)
        if (std::hypot(point.x()-positions[i].x(),point.y()-positions[i].y()) <= 28) return i;
    return -1;
}

void GraphWidget::resizeEvent(QResizeEvent *event) {
    if (!positions.empty() && event->oldSize().width()>0 && event->oldSize().height()>0) {
        for (auto &p : positions) {
            p.setX(65+(p.x()-65)*std::max(1,width()-130)/static_cast<qreal>(std::max(1,event->oldSize().width()-130)));
            p.setY(68+(p.y()-68)*std::max(1,height()-136)/static_cast<qreal>(std::max(1,event->oldSize().height()-136)));
        }
    }
    QWidget::resizeEvent(event);
    positionNameEditor(); positionWeightEditor();
}

void GraphWidget::beginConnection(int node, Qt::MouseButton button, QPointF worldPosition) {
    connectingNode = node;
    connectionButton = button;
    connectionStart = connectionCursor = worldPosition;
    connectionDragged = false;
    emit interactionStarted();
}

void GraphWidget::mousePressEvent(QMouseEvent *event) {
    ensurePositions();
    if(event->button()==Qt::MiddleButton) {
        finishNameEdit(); finishWeightEdit();
        panning=true; panStart=event->position(); setCursor(Qt::ClosedHandCursor);
        emit interactionStarted(); event->accept(); return;
    }
    suppressMouseContext = false;
    if (event->button() == Qt::RightButton) {
        connectingNode = nodeAt(toWorld(event->position()));
        if (connectingNode >= 0) {
            beginConnection(connectingNode, Qt::RightButton, toWorld(event->position()));
            event->accept(); return;
        }
    }
    if (event->button()==Qt::LeftButton) {
        finishNameEdit(); finishWeightEdit();
        setFocus(Qt::MouseFocusReason);
        draggedNode=nodeAt(toWorld(event->position()));
        const bool additive=event->modifiers() & (Qt::ControlModifier|Qt::ShiftModifier);
        if(draggedNode>=0 && selectedNodes.size()==1 && selectedNodes.contains(draggedNode) && !additive && !(event->modifiers() & Qt::AltModifier)) {
            const int node=draggedNode;
            draggedNode=-1;
            beginConnection(node, Qt::LeftButton, toWorld(event->position()));
            event->accept();return;
        }
        const QPointF world=toWorld(event->position());
        const int edge=draggedNode<0 ? edgeAt(world) : -1;
        if(draggedNode>=0) {
            if(additive && selectedNodes.contains(draggedNode)) {selectedNodes.erase(draggedNode);draggedNode=-1;update();return;}
            if(!additive && !selectedNodes.contains(draggedNode)) {selectedNodes.clear();selectedEdges.clear();}
            selectedNodes.insert(draggedNode);
        } else if(edge>=0) {
            const int key=edgeHits[edge].id;
            if(!additive) {selectedNodes.clear(); selectedEdges.clear();}
            if(additive && selectedEdges.contains(key)) selectedEdges.erase(key); else selectedEdges.insert(key);
            update();event->accept();return;
        } else {
            if(!additive) {selectedNodes.clear();selectedEdges.clear();}
            selectionBaseNodes=selectedNodes;selectionBaseEdges=selectedEdges;
            selecting=true;selectionStart=selectionEnd=world;emit interactionStarted();
        }
        update();
        if (draggedNode>=0) {
            dragOffset=positions[draggedNode]-toWorld(event->position());
            emit interactionStarted();
            setCursor(Qt::ClosedHandCursor); event->accept(); return;
        }
    }
    QWidget::mousePressEvent(event);
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event) {
    if(panning) {
        viewOffset+=event->position()-panStart;panStart=event->position();
        positionNameEditor();positionWeightEditor();update();event->accept();return;
    }
    if(selecting) {
        selectionEnd=toWorld(event->position());
        const QRectF box=QRectF(selectionStart,selectionEnd).normalized();
        selectedNodes=selectionBaseNodes; selectedEdges=selectionBaseEdges;
        for(int i=0;i<static_cast<int>(positions.size());++i) if(box.contains(positions[i])) selectedNodes.insert(i);
        for(const auto &h:edgeHits) {
            QPainterPathStroker stroker;stroker.setWidth(1/zoom);
            const bool intersects=box.intersects(h.box) || stroker.createStroke(h.path).intersects(box);
            if(intersects) selectedEdges.insert(h.id);
        }
        update();event->accept();return;
    }
    if (connectingNode >= 0 && (event->buttons() & connectionButton)) {
        connectionCursor = toWorld(event->position());
        if ((connectionCursor - connectionStart).manhattanLength() >= QApplication::startDragDistance())
            connectionDragged = true;
        if (connectionDragged) setCursor(Qt::CrossCursor);
        update(); event->accept(); return;
    }
    if (draggedNode>=0) {
        const auto p=toWorld(event->position())+dragOffset;
        const QPointF movement=p-positions[draggedNode];
        for(int id:selectedNodes) positions[id]+=movement;
        update(); event->accept(); return;
    }
    QWidget::mouseMoveEvent(event);
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event) {
    if(event->button()==Qt::MiddleButton && panning) {panning=false;unsetCursor();event->accept();return;}
    if(event->button()==Qt::LeftButton && selecting) {selecting=false;update();event->accept();return;}
    if (event->button() == connectionButton && connectingNode >= 0) {
        const int from = connectingNode, to = nodeAt(toWorld(event->position()));
        const bool dragged = connectionDragged ||
            (toWorld(event->position()) - connectionStart).manhattanLength() >= QApplication::startDragDistance();
        connectingNode = -1; connectionDragged = false;
        suppressMouseContext = connectionButton==Qt::RightButton; unsetCursor(); update();
        if (dragged) {
            if (to >= 0 && to != from) {
                const auto &edges = sim->graph[from].edges;
                const bool directed = sim->newEdgesDirected();
                const auto existing = std::find_if(edges.begin(), edges.end(),
                    [to, directed](const Edge &edge) { return edge.target == to && edge.directed == directed; });
                const auto undirected = std::find_if(edges.begin(), edges.end(),
                    [to](const Edge &edge) { return edge.target == to && !edge.directed; });
                // Preserve overlay-to-direct in mixed graphs; repeated connections
                // of the same kind create independent parallel edges.
                const bool convert = directed && existing == edges.end() && undirected != edges.end();
                const bool changed = convert ? sim->editEdge(from,undirected->id,undirected->weight,true)
                    : sim->addEdge(from,to,1,directed);
                if (changed) {
                    edgeHits.clear(); emit graphEdited();
                }
            }
        } else if(connectionButton==Qt::RightButton) {
            QContextMenuEvent menuEvent(QContextMenuEvent::Other, event->position().toPoint(), event->globalPosition().toPoint());
            contextMenuEvent(&menuEvent);
        }
        event->accept(); return;
    }
    if (event->button()==Qt::LeftButton && draggedNode>=0) {
        draggedNode=-1; unsetCursor();
        // Keep the displayed layout as the model layout for later additions/resizing.
        for (size_t i=0; i<positions.size(); ++i) {
            sim->graph[i].x=qRound(positions[i].x()); sim->graph[i].y=qRound(positions[i].y());
        }
        event->accept(); return;
    }
    QWidget::mouseReleaseEvent(event);
}

void GraphWidget::contextMenuEvent(QContextMenuEvent *event) {
    if (event->reason() == QContextMenuEvent::Mouse && (connectingNode >= 0 || suppressMouseContext)) {
        if (connectingNode < 0) suppressMouseContext = false;
        event->accept(); return;
    }
    ensurePositions();
    emit interactionStarted();
    QMenu menu(this); menu.setObjectName("t1");
    menu.setWindowFlag(Qt::FramelessWindowHint);
    menu.setAttribute(Qt::WA_TranslucentBackground);
    const QString menuStyle=R"(
        QMenu { background: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; border-radius: 12px; padding: 8px; font-family: 'Segoe UI'; font-size: 13px; }
        QMenu::item { padding: 10px 20px; border-radius: 6px; min-width: 180px; }
        QMenu::item:selected { background: #dbeafe; color: #1e40af; }
        QMenu::item:disabled { color: #64748b; }
        QMenu::separator { height: 1px; background: #e2e8f0; margin: 6px 8px; }
        QLabel { color: #0f172a; background: transparent; }
        QPushButton { background: #eff6ff; color: #1e40af; border: 1px solid #bfdbfe; border-radius: 6px; padding: 4px; min-width: 28px; min-height: 24px; }
        QPushButton:hover { background: #dbeafe; }
        QPushButton:disabled { background: #f1f5f9; color: #94a3b8; border-color: #e2e8f0; }
        QSpinBox { background: #f8fafc; color: #0f172a; border: 1px solid #94a3b8; border-radius: 6px; padding: 6px; min-height: 24px; }
        QSpinBox:focus { border-color: #2563eb; }
    )";
    menu.setStyleSheet(menuStyle);
    const QPointF world=toWorld(event->pos());
    const int node=nodeAt(world);
    // Use the same hit test as selection: both the weight and the actual
    // edge segment must open the edge menu, including after zoom/pan.
    const int hit = edgeAt(world);
    if (node<0 && hit>=0) {
        const int from=edgeHits[hit].from, edgeId=edgeHits[hit].id;
        int initial=0; bool directed=false;
        for (const auto &e : sim->graph[from].edges) if(e.id==edgeId) {initial=e.weight; directed=e.directed;}
        const std::set<int> actionEdges=selectedEdges.contains(edgeId) ? selectedEdges : std::set<int>{edgeId};
        bool removeDirection=directed;
        for(const auto &node:sim->graph) for(const auto &edge:node.edges)
            if(actionEdges.contains(edge.id) && edge.directed) removeDirection=true;
        auto *body=new QWidget; auto *rows=new QVBoxLayout(body); rows->setContentsMargins(10,8,10,8); rows->setSpacing(12);
        auto *adjust=new QHBoxLayout;
        adjust->addWidget(new QLabel("Trọng số"));
        auto *minus=new QPushButton("−"); minus->setObjectName("weightMinus");
        auto *value=new QLabel(QString::number(initial)); value->setMinimumWidth(40); value->setAlignment(Qt::AlignCenter);
        auto *plus=new QPushButton("+"); plus->setObjectName("weightPlus");
        adjust->addWidget(minus); adjust->addWidget(value); adjust->addWidget(plus); rows->addLayout(adjust);
        auto *inputRow=new QHBoxLayout; inputRow->addWidget(new QLabel("Nhập số"));
        auto *input=new QSpinBox; input->setObjectName("weightInput"); input->setRange(0,DijkstraSimulator::MaxEdgeWeight);
        input->setButtonSymbols(QAbstractSpinBox::NoButtons); input->setKeyboardTracking(false); input->setValue(initial);
        inputRow->addWidget(input); rows->addLayout(inputRow);
        auto refresh=[=](int w) { value->setText(QString::number(w)); minus->setEnabled(w>0); plus->setEnabled(w<DijkstraSimulator::MaxEdgeWeight); };
        refresh(initial);
        bool deleted = false;
        connect(input,&QSpinBox::valueChanged,&menu,[=, &deleted, this](int w) {
            if (deleted) return;
            if(sim->editEdge(from,edgeId,w,directed)) { refresh(w); emit graphEdited(); }
        });
        connect(minus,&QPushButton::clicked,&menu,[=] { input->setValue(input->value()-1); });
        connect(plus,&QPushButton::clicked,&menu,[=] { input->setValue(input->value()+1); });
        auto *action=new QWidgetAction(&menu); action->setDefaultWidget(body); menu.addAction(action);
        menu.addSeparator();
        menu.addAction("Xóa cạnh", [&, this] {
            deleted=true;
            bool changed=false;
            for(int id:actionEdges) {
                changed=sim->removeEdgeById(id)||changed;
                selectedEdges.erase(id);
            }
            if (changed) { edgeHits.clear(); emit graphEdited(); update(); }
        })->setObjectName("deleteEdge");
        if(removeDirection || sim->graphType==DijkstraSimulator::GraphType::Mixed)
        menu.addAction(removeDirection ? "Xóa hướng" : "Thêm hướng", [&,this] {
            input->interpretText();
            deleted=true;
            // Snapshot first: converting an edge can add/remove reverse arcs.
            std::vector<std::pair<int,Edge>> changes;
            for(const auto &node:sim->graph) for(const auto &edge:node.edges)
                if(actionEdges.contains(edge.id) && edge.directed==removeDirection &&
                   (edge.directed || node.id<edge.target)) changes.push_back({node.id,edge});
            bool changed=false;
            for(const auto &[owner,edge]:changes)
                changed=sim->editEdge(owner,edge.id,edge.weight,!removeDirection)||changed;
            if(changed) {
                // Removing one direction from a directed graph makes it mixed;
                // retain every other edge rather than converting the whole graph.
                if(removeDirection && sim->graphType==DijkstraSimulator::GraphType::Directed)
                    sim->graphType=DijkstraSimulator::GraphType::Mixed;
                edgeHits.clear(); emit graphEdited(); update();
            }
        })->setObjectName("toggleEdgeDirection");
        menu.exec(event->globalPos());
        // Commit keyboard edits even when the user dismisses the popup by clicking outside.
        if (!deleted) input->interpretText();
    } else if (node>=0) {
        menu.addAction("Đặt làm điểm xuất phát",[this,node] { sim->startNode=node; sim->reset(); emit graphEdited(); });
        menu.addAction("Đặt làm điểm kết thúc",[this,node] { sim->targetNode=node; sim->reset(); emit graphEdited(); });
        menu.addAction("Bỏ làm điểm kết thúc", [this] {
            sim->targetNode = -1; sim->reset(); emit graphEdited();
        })->setObjectName("clearTarget");
        menu.exec(event->globalPos());
    } else {
        const QPointF location=world;
        auto *add=menu.addAction("+  Thêm đỉnh tại đây",[this,location] {
            if(sim->addNode(qRound(location.x()),qRound(location.y()))) {
                positions.push_back(location);
                edgeHits.clear(); emit graphEdited();
            }
        });
        add->setEnabled(sim->graph.size()<DijkstraSimulator::MaxRandomNodes);
        menu.exec(event->globalPos());
    }
    event->accept();
}

bool GraphWidget::isEditing() const { return editingNode >= 0 || editingFrom >= 0; }

void GraphWidget::setInstantResult(bool enabled) {
    instantResult = enabled;
    if (enabled) animateStep(0, true);
    update();
}

void GraphWidget::positionNameEditor() {
    if (editingNode < 0 || editingNode >= static_cast<int>(positions.size())) return;
    const QPointF center = toScreen(positions[editingNode]);
    nameEditor->setGeometry(qRound(center.x())-NameWidth/2-2, qRound(center.y())-NameHeight/2, NameWidth+4, NameHeight);
}

void GraphWidget::finishNameEdit(bool cancel) {
    if (editingNode < 0) return;
    const int node = editingNode;
    const QString name = nameEditor->text().trimmed();
    const bool validName = nameEditor->hasAcceptableInput();
    editingNode = -1; // Focus-out generated by hide must not commit a second time.
    nameEditor->hide();
    if (!cancel && node < static_cast<int>(sim->graph.size()) && name != QString::fromStdString(sim->graph[node].name)) {
        if (validName && sim->renameNode(node, name.toStdString())) emit graphEdited();
        else QToolTip::showText(mapToGlobal(toScreen(positions[node]).toPoint()), "Tên tối đa 4 ký tự, phải vừa vòng tròn, không được để trống hoặc trùng. Đã giữ tên cũ.", this);
    }
    update();
}

bool GraphWidget::eventFilter(QObject *object, QEvent *event) {
    if(object==weightEditor || object==weightEditor->findChild<QLineEdit*>()) {
        if(event->type()==QEvent::ShortcutOverride) {event->accept();return true;}
        if(event->type()==QEvent::KeyPress) {
            const int key=static_cast<QKeyEvent*>(event)->key();
            if(key==Qt::Key_Escape || key==Qt::Key_Return || key==Qt::Key_Enter) {
                finishWeightEdit(key==Qt::Key_Escape);setFocus();return true;
            }
        }
        if(event->type()==QEvent::FocusOut) finishWeightEdit();
    }
    if (object == nameEditor) {
        if (event->type() == QEvent::ShortcutOverride) {
            event->accept(); return true;
        }
        if (event->type() == QEvent::KeyPress) {
            const auto *key = static_cast<QKeyEvent*>(event);
            if (key->key() == Qt::Key_Escape || key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
                finishNameEdit(key->key() == Qt::Key_Escape); setFocus(); return true;
            }
        }
        if (event->type() == QEvent::FocusOut) finishNameEdit();
    }
    return QWidget::eventFilter(object, event);
}

void GraphWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    ensurePositions();
    const int node = nodeAt(toWorld(event->position()));
    if (event->button() == Qt::LeftButton && node >= 0) {
        finishWeightEdit(); finishNameEdit();
        selectedNodes={node}; selectedEdges.clear();
        editingNode = node;
        draggedNode = connectingNode = -1; connectionDragged=false; unsetCursor();
        emit interactionStarted();
        nameEditor->setText(QString::fromStdString(sim->graph[node].name));
        positionNameEditor(); nameEditor->show(); nameEditor->raise();
        nameEditor->setFocus(Qt::MouseFocusReason); nameEditor->selectAll();
        update(); event->accept(); return;
    }
    if(event->button()==Qt::LeftButton) {
        const QPointF world=toWorld(event->position());
        const auto hit=std::find_if(edgeHits.rbegin(),edgeHits.rend(),[&](const EdgeHit &h){return h.box.contains(world);});
        if(hit!=edgeHits.rend()) {
            const int from=hit->from;
            finishNameEdit(); finishWeightEdit(); selecting=false; draggedNode=connectingNode=-1; connectionDragged=false; unsetCursor();
            emit interactionStarted(); editingFrom=from;
            editingEdgeId=hit->id;
            for(const auto &e:sim->graph[from].edges) if(e.id==editingEdgeId) weightEditor->setValue(e.weight);
            positionWeightEditor(); weightEditor->show(); weightEditor->raise(); weightEditor->setFocus(); weightEditor->selectAll();
            update();event->accept(); return;
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

void GraphWidget::keyPressEvent(QKeyEvent *event) {
    if ((event->key()==Qt::Key_Backspace || event->key()==Qt::Key_Delete) && !isEditing()) {
        emit interactionStarted(); bool changed=false;
        for(int id:selectedEdges) changed=sim->removeEdgeById(id)||changed;
        const auto nodes=selectedNodes;
        selectedEdges.clear(); selectedNodes.clear();
        for(auto it=nodes.rbegin();it!=nodes.rend();++it) {removeVertex(*it); changed=true;}
        if(changed) {edgeHits.clear(); emit graphEdited();}
        update(); event->accept(); return;
    }
    if(event->key()==Qt::Key_Escape) {selectedNodes.clear();selectedEdges.clear(); selecting=false;update();return;}
    if(event->key()==Qt::Key_A && event->modifiers() & Qt::ControlModifier) {
        for(const auto &n:sim->graph) selectedNodes.insert(n.id);
        for(const auto &h:edgeHits) selectedEdges.insert(h.id);
        update();return;
    }
    QWidget::keyPressEvent(event);
}

bool GraphWidget::removeVertex(int id) {
    finishNameEdit(true); finishWeightEdit(true);
    ensurePositions();
    emit interactionStarted();
    if (!sim->removeNode(id)) return false;
    draggedNode = connectingNode = -1;
    connectionDragged = false; unsetCursor();
    selectedNodes.clear(); selectedEdges.clear();
    positions.erase(positions.begin()+id);
    edgeHits.clear();
    emit graphEdited();
    return true;
}

int GraphWidget::edgeAt(QPointF point) const {
    // Labels take precedence over crossing segments.
    for(int i=static_cast<int>(edgeHits.size())-1;i>=0;--i)
        if(edgeHits[i].box.adjusted(-4/zoom,-4/zoom,4/zoom,4/zoom).contains(point)) return i;
    for(int i=static_cast<int>(edgeHits.size())-1;i>=0;--i) {
        const auto &h=edgeHits[i];
        QPainterPathStroker stroker;stroker.setWidth(12/zoom);
        stroker.setCapStyle(Qt::RoundCap);
        if(stroker.createStroke(h.path).contains(point)) return i;
    }
    return -1;
}
void GraphWidget::positionWeightEditor() {
    if(editingFrom<0) return;
    for(const auto &h:edgeHits) if(h.id==editingEdgeId) {
        const QPointF center=toScreen(h.box.center());
        QFont font("Segoe UI",10,QFont::DemiBold);font.setPixelSize(std::max(1,qRound(13*zoom)));weightEditor->setFont(font);
        const int width=std::max(20,qRound(h.box.width()*zoom)),height=std::max(12,qRound(h.box.height()*zoom));
        weightEditor->setGeometry(qRound(center.x())-width/2,qRound(center.y())-height/2,width,height);return;
    }
}
void GraphWidget::finishWeightEdit(bool cancel) {
    if(editingFrom<0) return;
    const int from=editingFrom,edgeId=editingEdgeId;
    editingFrom=-1;
    editingEdgeId=-1;
    if(!cancel) weightEditor->interpretText();
    const int weight=weightEditor->value();
    weightEditor->hide();
    if(!cancel && from<static_cast<int>(sim->graph.size())) {
        for(const auto &e:sim->graph[from].edges) if(e.id==edgeId) {
            if(e.weight!=weight && sim->editEdge(from,edgeId,weight,e.directed)) emit graphEdited();
            break;
        }
    }
    update();
}
void GraphWidget::zoomAt(qreal factor,QPointF anchor) {
    finishNameEdit();finishWeightEdit();ensurePositions();
    const QPointF world=toWorld(anchor);
    zoom=std::clamp(zoom*factor,0.2,4.0);
    viewOffset=anchor-world*zoom;
    emit zoomChanged(zoom);
    edgeHits.clear();update();
}
void GraphWidget::zoomBy(qreal factor) {zoomAt(factor,QPointF(width()/2.0,height()/2.0));}
void GraphWidget::resetView() {
    finishNameEdit();finishWeightEdit();zoom=1;viewOffset={};edgeHits.clear();update();
    emit zoomChanged(zoom);
}
void GraphWidget::wheelEvent(QWheelEvent *event) {
    zoomAt(std::pow(1.15,event->angleDelta().y()/120.0),event->position());event->accept();
}
