
#include "treegrid.h"
#include <map>
#include <utilfuncs/utilfuncs.h>


#define IMAGEFILE <TreeGrid/treegrid.iml>
#define IMAGECLASS TGPics
#include <Draw/iml.h>


//==============================================================================================
const Color inv_col(const Color C) { return Color(~C.GetR(), ~C.GetG(), ~C.GetB()); } //invert color

struct NodeLabelDisplay : public Display
{
	void Paint(Draw& w, const Rect& r, const Value& q, Color ink, Color paper, dword style) const
	{
		w.DrawRect(r, paper);
		std::string txt=q.ToString().ToStd();
		int h=(r.Height()-StdFont().Info().GetHeight())/2;
		size_t p=0;
		if ((p=txt.find(" -> "))!=std::string::npos)
		{
			std::string sl=txt.substr(0, p);
			std::string sr=txt.substr(p);
			LTRIM(sr, " ->");
			w.DrawText(r.left, r.top+h, sl.c_str(), StdFont(), ink);
			Size sz=GetTextSize(sl.c_str(), StdFont());
			Image pic=TGPics::PicSymLinkArrow();
			int hp=(r.Height()-pic.GetHeight())/2;
			w.DrawImage(r.left+sz.cx+5, r.top+hp, pic);
			w.DrawText(r.left+sz.cx+pic.GetWidth()+10, r.top+h, sr.c_str(), StdFont().Italic(), LtBlue());
		}
		else w.DrawText(r.left, r.top+h, q.ToString(), StdFont(), ink);
	}
};

//===================================================================================================================================================
Cell::~Cell() { clear(); }
Cell::Cell() { clear(); }
Cell::Cell(const std::string &v) : val(v), pDsp(nullptr) {}
Cell::Cell(const Cell &C) : val(C.val), pDsp(C.pDsp) {}

void Cell::operator=(const Cell &C)			{ val=C.val; pDsp=C.pDsp; }
void Cell::SetDisplay(const Display &d)		{ pDsp=&d; if (when_resync) when_resync(); }
const Display& Cell::GetDisplay() const		{ return *pDsp; }
void Cell::SetData(std::string v)			{ val=v; if (when_resync) when_resync(); if (WhenCellChanged) WhenCellChanged(); }
const std::string Cell::GetData() const		{ return val; }

//===================================================================================================================================================
Column::~Column() {}
Column::Column() : title(""), w(0), align(ALIGN_LEFT), b_asc(true), b_sort(false), dsp(&StdDisplay()) {}
Column::Column(const Column &C) : title(C.title), w(C.w), align(C.align), b_asc(C.b_asc), b_sort(C.b_sort), dsp(C.dsp), Sorter(C.Sorter) {}
Column::Column(const String &title, int width) : title(title), w(width), align(ALIGN_LEFT), b_asc(true), b_sort(false), dsp(&StdDisplay()) {}

Column& Column::Sorting(CompareNodes sortfunc)
{
	b_sort=true;
	Sorter=sortfunc;
	return *this;
}

bool Column::IsSorting() const { return b_sort; }
bool Column::IsASC() const { return b_asc; }
void Column::ToggleSort() { b_asc=!b_asc; }

Column& Column::Align(int align)
{
	this->align=align;
	if (align==ALIGN_RIGHT) dsp=&StdRightDisplay();
	else if (align==ALIGN_CENTER) dsp=&StdCenterDisplay();
	else dsp=&StdDisplay();
	return *this;
}

int Column::Align() const						{ return align; }
Column& Column::SetDisplay(const Display &d)	{ dsp=&d; return *this; }
const Display& Column::GetDisplay() const		{ return *dsp; }
const String& Column::Title() const				{ return title; }
void Column::Title(const String &title)			{ this->title=title; }
int Column::Width() const						{ return w; }
void Column::Width(int width)					{ w=width; }

//===================================================================================================================================================
node_pic_list node_pics{}; //?should be in Nodes? each? or in TGTree?-since it is used/displayed in the tree..

//===================================================================================================================================================
struct NODELIST : NoCopy
{
	std::map<PNode, int> mn{};
	~NODELIST() { for (auto& p:mn) delete p.first; mn.clear(); }
	NODELIST() { mn.clear(); }
	PNode Add(PNode pParent, const std::string &name, const std::string &key, Image pic)
	{
		PNode p=new Node(pParent, name, key, pic);
		if (p) mn[p]++;
		return p;
	}
	void Drop(PNode *pN)
	{
		if (*pN&&(mn.find(*pN)!=mn.end()))
		{
			mn[*pN]--;
			if (mn[*pN]<=0)
			{
				delete (*pN);
				mn.erase(*pN);
				*pN=nullptr;
			}
		}
	}
	PNode Copy(PNode pN)
	{
		if (pN&&(mn.find(pN)!=mn.end())) { mn[pN]++; return pN; }
		//return nullptr;
		throw std::logic_error("node does not exist");
	}
	///debug-aid...
	size_t count() { return mn.size(); }
	
};

NODELIST nodelist{};

//===================================================================================================================================================
void Nodes::clear()
{
	while (!vp.empty())
	{
		PNode p=(*vp.begin());
		vp.erase(vp.begin());
		nodelist.Drop(&p);

	}
	//vp.clear();
}

bool Nodes::empty() const							{ return vp.empty(); }
size_t Nodes::size() const							{ return vp.size(); }

Nodes::~Nodes()										{ clear(); }
Nodes::Nodes()										{ vp.clear(); }
Nodes::Nodes(const Nodes &R)						{ *this=R; }

Nodes& Nodes::operator=(const Nodes &R)
{
	clear();
	if (!R.empty()) { for (auto& p:R.vp) { vp.push_back(nodelist.Copy(p)); }}
	return *this;
}

Nodes::iterator Nodes::find(const std::string &key)
{
	iterator it=vp.begin();
	while (it!=vp.end()) { if ((*it)->GetKey()==key) break; else it++; }
	return it;
}

Nodes::const_iterator Nodes::find(const std::string &key) const
{
	const_iterator it=vp.begin();
	while (it!=vp.end()) { if ((*it)->GetKey()==key) break; else it++; }
	return it;
}

void Nodes::erase(const std::string &k)
{
	auto it=find(k);
	if (it!=end())
	{
		PNode pN=(*it);
		vp.erase(it);
		//pN->clear();
		nodelist.Drop(&pN);
	}
}

PNode Nodes::operator[](const std::string &key)
{
	auto it=find(key);
	if (it!=end()) return (*it);
	return nullptr; //throw std::out_of_range("invalid key");
}

const PNode Nodes::operator[](const std::string &key) const		{ return (*this)[key]; }

PNode Nodes::from_index(size_t i)
{
	if (i<size()) return vp[i];
	return nullptr; //throw std::out_of_range("invalid index");
}

const PNode Nodes::from_index(size_t i) const		{ return from_index(i); }
PNode Nodes::operator[](size_t i)					{ return from_index(i); }
const PNode Nodes::operator[](size_t i) const		{ return from_index(i); }
bool Nodes::has_key(const std::string &k)			{ return (find(k)!=end()); }

PNode Nodes::Add(PNode parent, const std::string &name, const std::string &key, Image pic)
{
	PNode pN=nullptr;
	std::string k{};
	if (key.empty()) { do { k=get_unique_name(name); } while (has_key(k)); }
	else { k=key; TRIM(k); }
	if (has_key(k)) throw std::logic_error("key not unique");
	if ((pN=nodelist.Add(parent, name, k, pic))) vp.push_back(pN);
	return pN;
}

void Nodes::Remove(PNode pN)						{ if (pN) erase(pN->GetKey()); }
void Nodes::Remove(const std::string &k)			{ erase(k); }

//===================================================================================================================================================
void Node::clear()
{
	cells.clear();
	skey.clear();
	sinfo.clear();
	y_pos=e_x_pos=p_x_pos=l_x_pos=0;
	picid=0;
	b_selected=false;
	ClearNodes();
	parent=nullptr;
	b_expandable=b_expanded=b_locked=false;
}

Node::~Node()										{ clear(); }
Node::Node()										{ picid=0; clear(); }
Node::Node(const Node &N)							{ *this=N; }

Node::Node(PNode pParent, const std::string &name, const std::string &key, Image pic)
{
	clear();
	parent=pParent;
	cells.push_back(Cell(name));
	cells[0].SetDisplay(Single<NodeLabelDisplay>());
	//todo:...fix: have internal key and user key if user key is invalid then ignore/or treat as list if dupe...
	//nkey for internal use, ukey for user ... don't need nkey-already using fixed pointers
	if (key.empty()) throw std::logic_error("key is empty");
	skey=key; //valid key expected - caller must validate..
	picid=node_pics.Add(pic);
}

Node& Node::operator=(const Node &N)
{
	clear();
	cells=N.cells;
	skey=N.skey;
	sinfo=N.sinfo;
	y_pos=N.y_pos;
	e_x_pos=N.e_x_pos;
	p_x_pos=N.p_x_pos;
	l_x_pos=N.l_x_pos;
	picid=N.picid;
	node_pics.IncPic(picid);
	b_selected=N.b_selected;
	nodes=N.nodes;
	parent=N.parent;
	b_expandable=N.b_expandable;
	b_expanded=N.b_expanded;
	b_locked=N.b_locked;
	return *this;
}

void Node::select(bool b)							{ b_selected=b; }

//size_t Node::AddCell(const std::string &v) //appends to cells returning index-pos; (displayed only up to column-count)
//{
//	if (v.empty()) cells.push_back(Cell("-")); else cells.push_back(Cell(v));
//	return (cells.size()-1);
//}
		
//void Node::RemoveCell(size_t idx)					{ if (idx<CellCount()) cells.erase(cells.begin()+idx); }
//size_t Node::CellCount() const						{ return cells.size(); } //may be > column-count
Cell& Node::CellAt(size_t idx)	{ if (idx<cells.size()) return cells.at(idx); throw std::out_of_range("invalid cell-index"); }

void Node::SetCellData(size_t idx, std::string s) { if (idx<cells.size()) cells.at(idx).SetData(s); }

void Node::ClearCells()			{ for (auto& c:cells) c.clear(); }
	
PNode Node::AddNode(const std::string &name, const std::string &key, Image pic)
{
	return nodes.Add(this, name, key, pic); //nullptr if key is not unique
}

void Node::DropNode(PNode pN)						{ DropNode(pN->GetKey()); }
void Node::DropNode(const std::string &key)			{ nodes.Remove(key); }
PNode Node::GetNode(const std::string &key)			{ return nodes[key]; }

std::string Node::GetNameKey(const std::string &sname)
{
	for (auto p:nodes) { if (seqs(sname, p->GetCellData(0))) return p->GetKey(); }
	return "";
}

std::string Node::GetInfoKey(const std::string &info)
{
	for (auto p:nodes) { if (seqs(info, p->sinfo)) return p->GetKey(); }
	return "";
}

PNode Node::GetNamedNode(const std::string &sname, bool bDeep)
{
	PNode pn=nullptr;
	std::string sk{};
	sk=GetNameKey(sname);
	if (!sk.empty()) pn=nodes[sk];
	else if (bDeep&&HasNodes()) { for (auto p:nodes) { if ((pn=p->GetNamedNode(sname, bDeep))) break; }}
	return pn;
}

PNode Node::GetInfoNode(const std::string &info, bool bDeep)
{
	PNode pn=nullptr;
	std::string sk{};
	sk=GetInfoKey(info);
	if (!sk.empty()) pn=nodes[sk];
	else if (bDeep&&HasNodes()) { for (auto p:nodes) { if ((pn=p->GetInfoNode(info, bDeep))) break; }}
	return pn;
}

void Node::ClearNodes()								{ if (HasNodes()) nodes.clear(); }
PNode Node::Parent() const							{ return parent; }
size_t Node::NodeCount() const						{ return nodes.size(); }
bool Node::HasNodes() const							{ return !nodes.empty(); }
PNode Node::SetInfo(const std::string &s)			{ sinfo=s; return this; }
const std::string Node::GetInfo() const				{ return sinfo; }
const std::string Node::GetKey() const				{ return skey; }
bool Node::IsSelected() const						{ return b_selected; }
PNode Node::SetLocked(bool b)						{ b_locked=b; return this; }
bool Node::IsLocked() const							{ return b_locked; }
void Node::Lock()									{ SetLocked(true); }
void Node::Unlock()									{ SetLocked(false); }
bool Node::IsExpandable() const						{ return b_expandable; }
PNode Node::Expandable(bool b)						{ b_expandable=b; return this; }
bool Node::IsExpanded() const						{ return b_expanded; }
bool Node::CanExpand() const						{ return (IsExpandable()&&!IsLocked()); }
void Node::Expand()									{ if (CanExpand()) b_expanded=true; }
void Node::Contract()								{ if (CanExpand()) b_expanded=false; }
bool Node::GetPic(Image &img) const					{ return node_pics.Get(picid, img); }
void Node::SetPic(const Image &img)					{ node_pics.Drop(picid); picid=node_pics.Add(img); }

int Node::get_indent() const
{
	int n=0;
	PNode p=Parent();
	while (p) { n++; p=p->Parent(); }
	return n;
}

//===================================================================================================================================================
TGHeader::~TGHeader()								{ Clear(); }
TGHeader::TGHeader()								{ Clear(); owner=nullptr; }
void TGHeader::Clear()								{ cols.clear(); CurCol=size_col=0; left_down=0; b_is_sizing=b_show=false; }
void TGHeader::show(bool b)							{ b_show=b; Show(b); }
bool TGHeader::show()								{ return b_show; }

void TGHeader::Paint(Draw &drw)
{
	Size sz=GetSize();
	drw.DrawRect(sz, SColorFace());
	Image picsort;
	Color fg=Black(), bg=SColorFace();

	if (cols.size()>0)
	{
		size_t i;
		int x, w, h=G_LINE_HEIGHT, px, py, pw;//, xd, yd, align;
	
		for (i=0;i<cols.size();i++)
		{
			if (cols[i].Width()>1)
			{
				//text=cols[i].title.c_str();
				//align=cols[i].Align();
				x=x_pos(i);
				w=cols[i].Width();
				px=py=pw=0;
				
				drw.DrawRect(x, 0, w, h, bg);
				drw.DrawLine(x, 0, x, h, 1, White());
				drw.DrawLine(x+w-1, 0, x+w-1, h, 1, Black()); //show "sizer"
	
				if (i==owner->sort_col)
				{
					picsort=(cols[i].IsASC())?TGPics::ITGASC():TGPics::ITGDSC();
					py=(G_LINE_HEIGHT-picsort.GetHeight())/2; //vert-center
					px=x+w-picsort.GetWidth()-3; //3-pixels away from R side
					pw=picsort.GetWidth();
				}
	
				if (!cols[i].Title().IsEmpty())
				{
					const Display &dsp=cols[i].GetDisplay();
					Rect r(x+3, 0, x+w-pw-3, h); //3-pixels away from L & R sides
					dsp.Paint(drw, r, cols[i].Title(), fg, bg, 0);
				}
				if (px&&py) drw.DrawImage(px, py, picsort);
			}
		}
	}
}

void TGHeader::scroll_x(int SBPos)
{
	if (SBPos>=0) SBPos=0;
	//SetRectX(SBPos, GetSize().cx-SBPos);
	Refresh();
}

bool TGHeader::is_on_sizer(int MX) const
{
	size_t t;
	return IsSizer(MX, t);
}

Image TGHeader::CursorImage(Point p, dword) // keyflags)
{
	if (b_is_sizing||is_on_sizer(p.x)) return TGPics::ITGSizer();
	return Image::Arrow();
}

void TGHeader::LeftDown(Point p, dword) // keyflags)
{
	size_t idx;
	if (b_is_sizing)
	{
		left_down=0;
		b_is_sizing=false;
		size_col=0;
		if (HasCapture()) ReleaseCapture();
	}
	else if (IsSizer(p.x, idx))
	{
		size_col=idx;
		left_down=p.x;
		b_is_sizing=true;
		SetCapture();
	}
	else
	{
		if (set_cur_col(p.x, idx)) WhenSorting(idx);
		Refresh();
	}
}

void TGHeader::MouseMove(Point p, dword) // keyflags)
{
	if (b_is_sizing)
	{
		if (p.x>x_pos(size_col))
		{
			int w=cols[size_col].Width();
			cols[size_col].Width(w+p.x-left_down);
			Refresh();
			owner->Refresh();
		}
		left_down=p.x;
	}
}

void TGHeader::LeftUp(Point, dword) //(Point p, dword keyflags)
{
	if (HasCapture()) ReleaseCapture();
	if (b_is_sizing)
	{
		left_down=0;
		b_is_sizing=false;
		size_col=0;
		owner->tree.SBH.SetTotal(Width()+5);
	}
}

Column& TGHeader::Add(const String &title, int width)
{
	size_t i=cols.size();
	cols.push_back(Column(title, width));
	return cols[i];
}

Column* TGHeader::ColumnAt(size_t idx) { if (idx<cols.size()) return &cols[idx]; else return nullptr; }

int TGHeader::x_pos(size_t idx)
{
	int x=-owner->tree.SBH;
	size_t i=0;
	while (i<cols.size()) { if (i==idx) break; x+=cols[i++].Width(); }
	return x;
	
}

int TGHeader::Width()
{
	int n=0;
	if (cols.size()>0) { for (auto& c:cols) n+=c.Width(); }
	return n;
}

bool TGHeader::IsSizer(int MX, size_t &idx) const
{
	int w=-owner->tree.SBH;
	for (idx=0;idx<cols.size();idx++)
	{
		w+=cols[idx].Width();
		if ((MX>=w-2)&&(MX<=w+2)) return true;
	}
	return false;
}

bool TGHeader::col_at_x(int MX, size_t &idx)
{
	int w=-owner->tree.SBH;
	for (idx=0;idx<cols.size();idx++)
	{
		w+=cols[idx].Width();
		if (MX<w) return true;
	}
	return false;
}

bool TGHeader::set_cur_col(int MX, size_t &idx)
{
	if (IsSizer(MX, idx)) return false;
	if (col_at_x(MX, idx))
	{
		if (CurCol==idx) cols[idx].ToggleSort();
		CurCol=idx;
		if (cols[idx].IsSorting()) return true; else return false;
	}
	return false;
}

//===================================================================================================================================================
TGTree::TGTree()
{
	Clear();
	WantFocus(true);
	
	//roots.Expanding(true); - each entry(==node) at root must be set individually
	
	//AddFrame(ThinInsetFrame()); - using blackframe in tgw..
	
	AddFrame(SBH);
	SBH.WhenScroll = THISBACK(DoHScroll);
	SBH.SetLine(1);
	SBH.AutoHide();

	AddFrame(SBV);
	SBV.WhenScroll = THISBACK(DoVScroll);
	SBV.SetLine(G_ROW_CY);
	SBV.AutoHide();
}

TGTree::~TGTree() { Clear(); }

void TGTree::Clear()
{
	roots.clear();
	pCurNode=pSelPivot=nullptr;
	lines_total_height=0;
	bShowTreeLines=true;
	exd=EXD_NONE;
	cur_column=0;
}

void TGTree::Layout()
{
	Size sz=GetSize();
	SBV.SetPage(sz.cy);
	SBH.SetPage(sz.cx);
}

void TGTree::Refresh()
{
	Layout();
	Ctrl::Refresh();
}

void TGTree::Paint(Draw &drw)
{
	Size szV=GetSize();
	drw.DrawRect(szV, G_COL_BG_DEFAULT);

	if (owner->header.cols.size()>0)
	{
		lines_total_height=show_nodes(drw, roots, szV, 0);
		if (bShowTreeLines) { for (auto& p:roots) { if (p->IsExpanded()&&p->HasNodes()) show_tree_lines(drw, p, szV); }}
		if (owner->b_vert_grid) { show_vert_grid(drw, szV); }
		SBV.SetTotal(lines_total_height);
		SBH.SetTotal(owner->header.Width()+5); //--- causes app to crash todo:...fix sometime... ?seems to work ok?
	}
}

int TGTree::show_nodes(Draw &drw, Nodes &nodes, Size szV, int Y)
{
	int	YY=Y,LC=((G_ROW_CY>G_LINE_HEIGHT)?((G_ROW_CY-G_LINE_HEIGHT)/2):0);
	int Y_MIN=(-G_ROW_CY);
	int Y_MAX=(szV.cy+G_ROW_CY);
	Color bg, fg;
	auto focus_box=[&](const Rect &r, Color col, int lwidth=2)
					{
						drw.DrawLine(r.left, r.top, r.right, r.top, lwidth, col);
						drw.DrawLine(r.left, r.top, r.left, r.bottom-lwidth, lwidth, col);
						drw.DrawLine(r.right-lwidth, r.top, r.right-lwidth, r.bottom-lwidth, lwidth, col);
						drw.DrawLine(r.left, r.bottom-lwidth, r.right, r.bottom-lwidth, lwidth, col);
					};

	for (auto& N:nodes)
	{
		if ((!N->Parent()||N->Parent()->IsExpanded()) && ((YY-SBV)>Y_MIN) && ((YY-SBV)<Y_MAX)) //(N->y_pos>Y_MIN) && (N->y_pos<Y_MAX))
		{
			int x=0, y=N->y_pos, w=0, lrpad=3;//lrpad keeps text at least 3 pixels away from L & R edges
			int YC=((y+LC)-SBV), YS=y-SBV; //YC==v-center-top, YS==top-edge (both of current row)
	
			if (N->IsLocked())
			{
				//if (Y)
				drw.DrawImage(N->e_x_pos-SBH, YC, TGPics::ITGLock());
				//else drw.DrawImage(N->e_x_pos-SBH, YC, TGPics::ITGBlocked()); if Y==0 then pinexpanded?
			}
			else if (N->IsExpandable())
			{
				if (N->IsExpanded()) drw.DrawImage(N->e_x_pos-SBH, YC, TGPics::ITGContract());
				else drw.DrawImage(N->e_x_pos-SBH, YC, TGPics::ITGExpand());
			}
			else drw.DrawImage(N->e_x_pos-SBH, YC, TGPics::ITGLeaf());
			
			//node-picture...
			Image np;
			if (N->GetPic(np)) drw.DrawImage(N->p_x_pos-SBH, YC, np);
	
			//foreground & background colors
			fg=G_COL_FG_DEFAULT;
			bg=G_COL_BG_DEFAULT;
			if (N==owner->tree.pCurNode) { bg=G_COL_BG_FOCUS; fg=G_COL_FG_FOCUS; }
			else if (N->IsSelected()) { bg=G_COL_BG_SELECTED; fg=G_COL_FG_SELECTED; }
			else
			{
				int indent=N->get_indent();
				switch (indent%3)
				{
					case 0: bg=(((YY/G_ROW_CY)%2)==0)?G_PASTEL_ONE:G_PASTEL_ONE_ALT; break;
					case 1: bg=(((YY/G_ROW_CY)%2)==0)?G_PASTEL_TWO:G_PASTEL_TWO_ALT; break;
					case 2: bg=(((YY/G_ROW_CY)%2)==0)?G_PASTEL_THREE:G_PASTEL_THREE_ALT; break;
				}
			}
			drw.DrawRect(N->l_x_pos-SBH, YS, szV.cx+SBH, G_ROW_CY, bg); //NB: using YS

			const Display &dsp=(N->cells[0].pDsp)?N->cells[0].GetDisplay():owner->header.cols[0].GetDisplay();
			Rect r(N->l_x_pos-SBH, YS, (N->l_x_pos+owner->header.cols[0].Width()-SBH-lrpad), YS+G_ROW_CY);
			dsp.Paint(drw, r, N->cells[0].GetData().c_str(), fg, bg, 0);

			//cell-focus-box
			if ((cur_column==0)&&(N==owner->tree.pCurNode)) focus_box(Rect(N->l_x_pos-2, YS, owner->header.cols[0].Width(), YS+G_ROW_CY), inv_col(bg), 3);
			
			//cell-contents...
			for (size_t i=1;i<N->cells.size();i++)
			{
				if (i>=owner->header.cols.size()) continue;
				x=owner->header.x_pos(i);
				w=owner->header.cols[i].Width();
				drw.DrawRect(x, YS, w, G_ROW_CY, bg); //NB: YS
				
				//nodes-display
				const Display &dsp=(N->cells[i].pDsp)?N->cells[i].GetDisplay():owner->header.cols[i].GetDisplay();
				Rect r(x+lrpad, YS, x+w-lrpad, YS+G_ROW_CY);
				dsp.Paint(drw, r, N->cells[i].GetData().c_str(), fg, bg, 0);

				if ((i==cur_column)&&(N==owner->tree.pCurNode)) focus_box(Rect(x+1, YS, x+w, YS+G_ROW_CY), inv_col(bg), 3);
			}
		}
		YY+=G_ROW_CY; //need total for vscrollbar
		if (N->IsExpanded()) YY=show_nodes(drw, N->nodes, szV, YY);
	}
	return YY;
}

void TGTree::show_tree_lines(Draw &drw, PNode N, Size szV)
{
	if (!N->HasNodes()) return;
	PNode A=(*N->nodes.begin());
	PNode Z=(*N->nodes.rbegin());
	int x,yt,yb,indent=A->get_indent();
	Color col=(indent%2)?Gray():LtGray();

	x=(((indent-1)*G_SPACE)+G_GAP+1);
	if ((x+5)<owner->header.ColumnAt(0)->Width())
	{
		if ((x-=SBH)>0)
		{
			yt=max((A->y_pos-SBV),0);
			yb=min((Z->y_pos+G_ROW_CY/2-1-SBV), szV.cy);
			if (A->Parent())
			{
				Color bg;
				if (A->Parent()==pCurNode) bg=G_COL_BG_FOCUS;
				else
				{
					switch(indent%3)
					{
						case 0: bg=G_PASTEL_ONE_ALT; break;
						case 1: bg=G_PASTEL_TWO_ALT; break;
						case 2: bg=G_PASTEL_THREE_ALT; break;
					}
				}
				drw.DrawRect(x-G_GAP, yt, G_SPACE, (yb-yt), bg);
			}
			if ((yt>=0)&&(yb<=szV.cy))
			{
				drw.DrawLine(x, yt, x, yb, 1, col);
				drw.DrawLine(x, yb, (x+5), yb, 1, col);
			}
		}
		for (auto& p:N->nodes) { if (p->IsExpanded()) show_tree_lines(drw, p, szV); }
	}
}

void TGTree::show_vert_grid(Draw &drw, Size szV)
{
	PNode pN=owner->tree.get_last_node();
	if (pN)
	{
		int X=-SBH, B=min((pN->y_pos+G_ROW_CY-1),szV.cy);
		size_t i=0;
		while (i<owner->header.cols.size())
		{
			X+=owner->header.cols[i].Width();
			if ((X>0)&&(X<szV.cx)) drw.DrawLine(X, 0, X, B, 1, SColorFace());
			i++;
		}
	}
}

int TGTree::set_tree_positions(Nodes &nodes, int Y)
{
	int	y=Y, i;
	auto it=nodes.begin();
	while (it!=nodes.end())
	{
		PNode N=(*it);
		//if (N->IsExpanded())
		//{
			int x=1; //using x=0 to find nodes will toggle expand/contract
			N->y_pos=y;
			for (i=0;i<N->get_indent();i++) x+=G_SPACE;
			N->e_x_pos=x; //pos of empty, leaf or expand/contract picture
			x+=G_SPACE+G_GAP;
			if (N->picid) { N->p_x_pos=x; x+=G_PIC_WIDTH+G_SPACE; } //pic-pos
			N->l_x_pos=x; //label/text-position
			y+=G_ROW_CY;
			if (N->IsExpanded()) y=set_tree_positions(N->nodes, y);
		//}
		it++;
	}
	return y;
}

PNode TGTree::get_node(PNode pN, const String &key)
{
	if (!pN->HasNodes()||key.IsEmpty()) return nullptr;
	if (pN->nodes.has_key(key.ToStd())) { return pN->nodes[key.ToStd()]; }
	PNode pk=nullptr;
	auto it=pN->nodes.begin();
	while (!pk&&(it!=pN->nodes.end())) { if ((pk=get_node((*it), key))) break; else it++; }
	return pk;
}

PNode TGTree::get_last_node()
{
	PNode pn=nullptr;
	if (!roots.empty())
	{
		pn=roots[roots.size()-1];
		while (pn->IsExpanded()&&(pn->NodeCount()>0)) { pn=pn->nodes[pn->NodeCount()-1]; }
	}
	return pn;
}

void TGTree::sync_nodes()	{ set_tree_positions(roots, 0); Refresh(); }

PNode TGTree::AddNode(PNode pParent, const String &treetext, const String &key)
{
	PNode pN=nullptr;
	if (pParent) pN=pParent->AddNode(treetext.ToStd(), key.ToStd());
	else pN=roots.Add(nullptr, treetext.ToStd(), key.ToStd(), Image());
	sync_nodes();
	return pN;
}

PNode TGTree::AddNode(PNode pParent, Image pic, const String &treetext, const String &key)
{
	PNode pN=nullptr;
	if (pParent) pN=pParent->AddNode(treetext.ToStd(), key.ToStd(), pic);
	else pN=roots.Add(nullptr, treetext.ToStd(), key.ToStd(), pic);
	sync_nodes();
	return pN;
}

PNode TGTree::get_node(Nodes &nodes, const std::string &k)
{
	if (nodes.empty()) return nullptr;
	PNode pn=nodes[k];
	if (!pn)
	{
		auto it=nodes.begin();
		while (!pn&&(it!=nodes.end())) { if (!(pn=get_node((*it)->nodes, k))) it++; }
	}
	return pn;
}

PNode TGTree::GetNode(const String &key)
{
	return get_node(roots, key.ToStd());
}

PNode TGTree::get_named_node(Nodes &nodes, const std::string &sname)
{
	if (nodes.empty()) return nullptr;
	PNode pn=nullptr;
	for (auto p:nodes)
	{
		if ((pn=p->GetNamedNode(sname))) break;
	}
	return pn;
}

PNode TGTree::get_info_node(Nodes &nodes, const std::string &info)
{
	if (nodes.empty()) return nullptr;
	PNode pn=nullptr;
	for (auto p:nodes)
	{
		if ((pn=p->GetInfoNode(info))) break;
	}
	return pn;
}

PNode TGTree::GetNamedNode(const String &sname)					{ return get_named_node(roots, sname.ToStd()); }
PNode TGTree::GetInfoNode(const String &info)					{ return get_info_node(roots, info.ToStd()); }

void TGTree::SelectNode(PNode N, bool bSelect)
{
	if (N)
	{
		N->select(bSelect);
		if (bSelect) b_has_selected=true; //don't unset b_has_selected - if it is SET leave it SET
	}
}

void TGTree::SelectNode(const String &key, bool bSelect)
{
	SelectNode(GetNode(key), bSelect);
}

void TGTree::SelectRange(PNode N1, PNode N2)
{
	if (N1&&N2)
	{
		unselect_nodes(roots);
		PNode t=N1, b=N2, n;
		if (b->y_pos<t->y_pos) { n=t; t=b; b=n; }
		n=t;
		while (n&&(n!=b))
		{
			SelectNode(n, true);
			n=get_node_at_y((n->y_pos+G_ROW_CY+1), roots);
		}
		b->select();
		b_has_selected=true;
	}
}

void TGTree::Lock(PNode N, bool b)
{
	if (N)
	{
		if (b&&N->IsLocked()) return;
		N->SetLocked(b);// nodes.b_locked=b;
		Refresh();
	}
}

void TGTree::DropNodes(PNode N) { if (N) N->nodes.clear(); }
void TGTree::DropNode(PNode N)
{
	if (N) { PNode pn=N->Parent(); if (pn) pn->DropNode(N->GetKey()); }
}

void TGTree::DoVScroll()	{ Refresh(); }
void TGTree::DoHScroll()
{
	Refresh();
	//if(!owner->header.b_is_sizing)
	owner->header.scroll_x(-SBH);
}

PNode TGTree::get_y_node(int Y, PNode N)
{
	PNode pn=nullptr;
	auto Is_Y = [&](PNode n, int y)->PNode{ PNode r=nullptr; if (((n->y_pos)<=y)&&((n->y_pos+G_ROW_CY)>=y)) r=n; return r; };
	if (N&&!(pn=Is_Y(N, Y))&&N->IsExpanded())
	{
		auto it=N->nodes.begin();
		while (!pn&&(it!=N->nodes.end())) { if ((pn=get_y_node(Y, (*it)))) break; else it++; }
	}
	return pn;
}

PNode TGTree::get_node_at_y(int Y, Nodes &nodes)
{
	PNode pn=nullptr;
	for (auto& p:nodes) { if ((pn=get_y_node(Y, p))) break; }
	return pn;
}

bool TGTree::toggle_expand(PNode N, Point p) //toggle expand/contract if clicked on "button"
{
	if (!N||N->IsLocked()) return false;
	if (((p.x+SBH)>=(N->e_x_pos-G_SPACE))&&((p.x+SBH)<(N->e_x_pos+(G_SPACE+G_GAP))))
	{
		Expand(N, !N->IsExpanded());
		return true;
	}
	return false;
}

void TGTree::check_cur_node_contract(PNode N)
{
	if (pCurNode)
	{
		PNode pn=pCurNode->Parent();
		while (pn&&(pn!=N)) pn=pn->Parent();
		if (pn==N) FocusNode(N); //if ancestor - change focus
	}
}

void TGTree::Expand(PNode N, bool b)
{
	if (N)
	{
		if (b)
		{
			owner->OnBNE(N); //allow caller to adjust node before expanding
			N->Expand();
			sort_nodes(&owner->header, N->nodes, owner->sort_col, owner->header.cols[owner->header.CurCol].IsASC());
		}
		else
		{
			check_cur_node_contract(N);
			for (auto& p:N->nodes) Expand(p, false);
			N->Contract();
			owner->OnANC(N); //allow caller to cleanup after contracting
		}
		sync_nodes();
	}
}

void TGTree::FocusNode(PNode N, bool bView)
{
	if (N&&(pCurNode!=N))
	{
		pCurNode=N;
		if (pCurNode->Parent()&&!pCurNode->Parent()->IsExpanded()) Expand(pCurNode->Parent(), true);
		if (bView) BringToView(pCurNode);
		Refresh();
		if (bView) WhenFocus(pCurNode);
	}
}

void TGTree::BringToView(PNode N)
{
	if (N)
	{
		Size sz=GetSize();
		int XL=owner->header.x_pos(cur_column), XR=(XL+owner->header.ColumnAt(cur_column)->Width());
		SBV.ScrollInto(N->y_pos);
		if ((XL-SBH)<0) SBH.ScrollInto(XL); else if (XR>sz.cx) SBH.ScrollInto(XR);
		Refresh();
	}
}

void TGTree::sort_nodes(TGHeader *pH, Nodes &nodes, size_t c, bool b)
{
	if (pH->cols[c].Sorter) std::sort(nodes.begin(), nodes.end(), pH->cols[c].Sorter); //pH->cols[c].Sorter); //user's sorting func
	else std::sort(nodes.begin(),
					nodes.end(),
					[this, c, b](const PNode &l, const PNode &r) //dict-compare
						{
							auto X=[=](int x)->bool{ return (exd==x); };
							if (!X(EXD_NONE))
							{
								if (l->CanExpand()&&!r->CanExpand()) return (X(EXD_FIRST)?true:false); //expandable nodes first
								if (!l->CanExpand()&&r->CanExpand()) return (X(EXD_FIRST)?false:true); // .. last
							}
							int n=sicmp(l->cells[c].GetData(), r->cells[c].GetData());
							return (b)?(n<0)?true:false:(n>0);
						});
	for (auto& p:nodes) sort_nodes(pH, p->nodes, c, b);
}

void TGTree::SortNodes(TGHeader *pH, size_t colidx, bool bASC)
{
	sort_nodes(pH, roots, colidx, bASC);
	sync_nodes();
}

void TGTree::unselect_nodes(Nodes &nodes)
{
	for (auto& p:nodes)
	{
		if (p->IsSelected()) p->select(false);
		unselect_nodes(p->nodes);
		b_has_selected=false;
	}
}

size_t TGTree::get_selected_keys(Nodes &nodes, std::vector<String> &V)
{
	if (!b_has_selected) return 0;
	auto it=nodes.begin();
	while (it!=nodes.end())
	{
		if ((*it)->IsSelected()) V.push_back((*it)->GetKey().c_str());
		get_selected_keys((*it)->nodes, V);
		it++;
	}
	return V.size();
}

size_t TGTree::get_selected_nodes(Nodes &nodes, std::vector<PNode> &V)
{
	if (!b_has_selected) return 0;
	auto it=nodes.begin();
	while (it!=nodes.end())
	{
		if ((*it)->IsSelected()) V.push_back((*it));
		get_selected_nodes((*it)->nodes, V);
		it++;
	}
	return V.size();
}

bool TGTree::contains_locked_nodes(PNode N)
{
	if (!N) return false;
	bool b=false;
	auto it=N->nodes.begin();
	while (!b&&(it!=N->nodes.end()))
	{
		if ((b=(*it)->IsLocked())) break;
		if (!(b=contains_locked_nodes((*it)))) it++;
	}
	return b;
}

bool TGTree::Key(dword key, int)
{
	bool shift=key&K_SHIFT;
	bool ctrl=key&K_CTRL;
	bool alt=key&K_ALT;

	if (shift) key&=~K_SHIFT;
	if (ctrl) key&=~K_CTRL;
	if (alt) key&=~K_ALT;

	switch(key)
	{
		case K_F1: { if (owner->WhenHelp) owner->WhenHelp(); } break; //else owner->OnTGHelp(); return true; } break;
		case K_ENTER: { if (pCurNode) { WhenLeftDouble(pCurNode, cur_column); return true; }} break;
		case K_UP: case K_DOWN:
			{
				if (pCurNode)
				{
					PNode pn=get_node_at_y((key==K_UP)?(pCurNode->y_pos-G_ROW_CY+1):(pCurNode->y_pos+G_ROW_CY+1), roots);
					if (pn)
					{
						if (shift)
						{
							if (!pSelPivot) pSelPivot=pCurNode;
							owner->SelectRange(pSelPivot, pn);
						}
						else
						{
							unselect_nodes(roots);
							pSelPivot=nullptr;
						}
						FocusNode(pn);
					}
					return true;
				}
				else return SBV.VertKey(key);
			} break;
		case K_PAGEUP: case K_PAGEDOWN:
			{
				owner->ClearSelection();
				pSelPivot=nullptr;
				if (pCurNode)
				{
					Size sz=GetSize();
					int y=pCurNode->y_pos-SBV+(sz.cy%G_ROW_CY);
					if (key==K_PAGEUP) SBV=SBV-sz.cy; else SBV=SBV+sz.cy;
					PNode pn=get_node_at_y(y+SBV, roots);
					FocusNode(pn);
					return true;
				}
				else return SBV.VertKey(key);
			} break;
		case K_HOME: case K_END:
			{
				if (roots.size()>0)
				{
					if (ctrl)
					{
						owner->ClearSelection();
						pSelPivot=nullptr;
						PNode N=(key==K_HOME)?roots[0]:get_last_node();
						FocusNode(N);
						return true;
					}
					else if (pCurNode) { cur_column=(key==K_HOME)?0:(owner->header.cols.size()-1); BringToView(pCurNode); return true; }
				}
				return SBH.VertKey(key);
			} break;
		case K_LEFT:
			{
				if (pCurNode)
				{
					if (ctrl&&pCurNode->IsExpanded())
					{
						owner->ClearSelection();
						pSelPivot=nullptr;
						Expand(pCurNode, false);
					}
					else { cur_column-=(cur_column>0)?1:0; BringToView(pCurNode); }
					return true;
				}
				return SBH.VertKey(key);
			} break;
		case K_RIGHT:
			{
				if (pCurNode)
				{
					if (ctrl&&!pCurNode->IsExpanded())
					{
						owner->ClearSelection();
						pSelPivot=nullptr;
						Expand(pCurNode, true);
					}
					else { cur_column+=(cur_column<(owner->header.cols.size()-1))?1:0; BringToView(pCurNode); }
					return true;
				}
				else return SBH.VertKey(key);
			} break;
		case K_M: //popupmenu.. Ctrl+M
			{
				if (ctrl)
				{
					Rect r=get_abs_tree_rect(); ///works - chg to GetScreenRect() / GetScreenView() ... later maybe
					Point p;
					if (pCurNode)
					{
						p.x=r.left+(owner->header.x_pos(cur_column)+owner->header.ColumnAt(cur_column)->Width()/2)-SBH;
						p.y=r.top+(pCurNode->y_pos+G_ROW_CY/2)-SBV;
					}
					else
					{
						Size sz=GetSize(); //how to get dimensions of menu-popup? - for centering
						p.x=r.left+sz.cx/3;
						p.y=r.top+sz.cy/3; //default to top-left 1/3-pos
					}
					owner->ShowPopMenu(p);
				}
			} break;
		default: return SBV.VertKey(key); break;
	}
	return false;
}

Rect TGTree::get_abs_tree_rect()
{
	Ctrl *p=GetParent();
	Rect w, r=GetRect();
	while (p) { w=p->GetRect(); r.left+=w.left; r.top+=w.top; p=p->GetParent(); } //top parent (p==nullptr) should have absolute coord's?
	return r;
}

size_t TGTree::set_click_column(int x) { size_t cc=0; owner->header.col_at_x(x, cc); cur_column=cc; return cc; } //fix..

bool TGTree::point_in_tree(Point p) ///FIX? can as well return the node at p...who calls & why...also node_at_y()
{
	if (roots.size()==0) return false;
	if ((p.x<0)||(p.x>owner->header.Width())) return false;
	PNode pN=get_last_node();
	int yb=(pN->y_pos+G_ROW_CY);
	if ((p.y<0)||(p.y>yb)) return false;
	return true;
}

PNode TGTree::prev_node(PNode N)
{
	if (N) { return get_node_at_y((N->y_pos-G_ROW_CY+1), roots); }
	return nullptr;
}

PNode TGTree::next_node(PNode N)
{
	if (N) { return get_node_at_y((N->y_pos+G_ROW_CY+1), roots); }
	return nullptr;
}

void TGTree::MouseWheel(Point, int zdelta, dword)//(Point p, int zdelta, dword keyflags)
{
	SBV.Wheel(zdelta);
}

void TGTree::LeftDown(Point p, dword flags)
{
	size_t idx;
	if (owner->header.IsSizer(p.x, idx))
	{
		owner->header.LeftDown(p, flags);
	}
	else
	{
		if (!point_in_tree(p)) return;
		PNode N=get_node_at_y(p.y+SBV, roots);
		if (N)
		{
			if (toggle_expand(N, p)) Refresh();
			else
			{
				set_click_column(p.x);
				WhenLeftDown(N, flags);
			}
		}
	}
}

void TGTree::LeftUp(Point p, dword flags)
{
	if (owner->header.b_is_sizing) owner->header.LeftUp(p, flags);
}

void TGTree::RightDown(Point p, dword)// flags) pedantic
{
	if (point_in_tree(p))
	{
		PNode pn=get_node_at_y((p.y+SBV), roots);
		if (!pn->IsSelected()) unselect_nodes(roots);
		FocusNode(pn);
		set_click_column(p.x);
	}
	WhenRightDown(pCurNode); //pCurNode may be nullptr..
}

void TGTree::LeftDouble(Point p, dword)// flags)
{
	if (!point_in_tree(p)) return;
	PNode N=get_node_at_y(p.y+SBV, roots);
	unselect_nodes(roots);
	if (toggle_expand(N, p)) Refresh();
	else
	{
		set_click_column(p.x);
		WhenLeftDouble(N, cur_column);
	}
	//FocusNode(N);
}

void TGTree::MouseMove(Point p, dword flags)
{
	if (owner->header.b_is_sizing) owner->header.MouseMove(p, flags);
}

Image TGTree::CursorImage(Point p, dword flags)
{
	return owner->header.CursorImage(p, flags);
	//if (b_is_sizing) return TGPics::ISizer();
	//if (is_on_sizer(p.x)) return TGPics::ISizer();
	//return Image::Arrow();
}

//===================================================================================================================================================
TreeGrid::~TreeGrid() { Clear(); }

TreeGrid::TreeGrid()
{
	Clear(); //init's
	WantFocus(true);
	
	Add(header.HSizePosZ().TopPos(0, G_LINE_HEIGHT+4).AddFrame(ThinInsetFrame()));
	header.owner=this;
	header.WhenSorting = THISBACK(OnSorting);
	
	//Add(tree.HSizePosZ(0,0).VSizePosZ(0,G_LINE_HEIGHT));
	tree.owner=this;
	tree.WhenFocus << Proxy(WhenFocus);
	tree.WhenLeftDown << THISBACK(OnLeftDown);
	tree.WhenLeftDouble << THISBACK(OnLeftDouble);
	tree.WhenRightDown << THISBACK(OnRightDown);
	tree.WhenBeforeNodeExpand << THISBACK(OnBNE); //Proxy(WhenBeforeNodeExpand); ???
	tree.WhenAfterNodeContract << THISBACK(OnANC); //Proxy(WhenAfterNodeContract);
	
	AddFrame(BlackFrame());
	
}

void TreeGrid::Layout()
{
	int toppos=((header.show())?G_LINE_HEIGHT:0);
	Add(tree.HSizePosZ().VSizePosZ(toppos));
}

void TreeGrid::GotFocus()									{ tree.SetFocus(); }
void TreeGrid::OnBNE(PNode N)								{ WhenBeforeNodeExpand(N); }
void TreeGrid::OnANC(PNode N)								{ WhenAfterNodeContract(N); }

void TreeGrid::Clear()
{
	header.Clear();
	tree.Clear();
	sort_col=0;
	b_vert_grid=true;
	b_internal=false;
}

void TreeGrid::Select(PNode N, bool bSelect)				{ if (N) { tree.SelectNode(N, bSelect); Refresh(); }}
void TreeGrid::SelectRange(PNode N1, PNode N2)				{ tree.SelectRange(N1, N2); Refresh(); }
void TreeGrid::ClearSelection()								{ tree.unselect_nodes(tree.roots); Refresh(); }
size_t TreeGrid::GetSelectionKeys(std::vector<String> &v)	{ v.clear(); return tree.get_selected_keys(tree.roots, v); }
size_t TreeGrid::GetSelectionNodes(std::vector<PNode> &v)	{ v.clear(); return tree.get_selected_nodes(tree.roots, v); }
bool TreeGrid::Key(dword key, int r)						{ return tree.Key(key, r); }
void TreeGrid::OnSorting(size_t idx)						{ sort_col=idx; SyncNodes(); tree.BringToView(GetFocusNode()); }
size_t TreeGrid::GetSortColumnID()							{ return sort_col; }
bool TreeGrid::IsSortASC()									{ return header.cols[sort_col].IsASC(); }

Column& TreeGrid::AddColumn(const String title, int width)
{
	Column &C=header.Add(title, width);
	sync_all_node_cells(tree.roots);
	//header.show(true);
	return C;
}

void TreeGrid::sync_all_node_cells(Nodes &nodes)
{
	for (auto& N:nodes)
	{
		check_cells(N);
		sync_all_node_cells(N->nodes);
	}
}

void TreeGrid::check_cells(PNode N)
{
	if (N)
	{
		size_t n=N->cells.size();
		while (n<header.cols.size()) { N->cells.push_back(Cell("")); n++; }
		for (size_t i=0;i<header.cols.size();i++) N->CellAt(i).when_resync << [&](){ SyncNodes(); }; //sorting etc.
	}
}

PNode TreeGrid::AddNode(PNode pParent, const String &name, const String &key)
{
	PNode N=tree.AddNode(pParent, name, key);
	check_cells(N);
	return N;
}

void TreeGrid::SyncNodes()
{
	if (header.cols.size()>0)
	{
		if (header.cols[sort_col].IsSorting()) tree.SortNodes(&header, sort_col, header.cols[sort_col].IsASC());
		tree.sync_nodes();
	}
}

void TreeGrid::ClearTree()									{ tree.Clear(); }
void TreeGrid::RefreshTreeGrid()							{ SyncNodes(); Layout(); tree.Refresh(); }

void TreeGrid::DeleteNode(PNode N, bool bfocus)
{
	PNode pn=GetFocusNode();
	if (N==pn)
	{
		pn=GetNextNode(N);
		if (!pn) pn=GetPrevNode(N);
	}
	tree.DropNode(N);
	SetFocusNode(pn, bfocus);
}

PNode TreeGrid::GetNode(const String &key)					{ return tree.GetNode(key); }
PNode TreeGrid::GetNodeNamed(const String &S)				{ return tree.GetNamedNode(S); }
PNode TreeGrid::GetInfoNode(const String &info)				{ return tree.GetInfoNode(info); }
PNode TreeGrid::RootAt(size_t idx)							{ return ((idx<tree.roots.size())?tree.roots[idx]:nullptr); }
size_t TreeGrid::RootCount()								{ return tree.roots.size(); }

void TreeGrid::LockNode(PNode N, bool bExpand)
{
	if (!N) return;
	if (bExpand) { Expand(N, bExpand); tree.Lock(N, true); }
	else { tree.Lock(N, true); Expand(N, bExpand); } //??wtf
}

void TreeGrid::UnlockNode(PNode N)							{ if (N) tree.Lock(N, false); }
void TreeGrid::Expand(PNode N, bool b)						{ if (N) { tree.Expand(N, b); SyncNodes(); }}
void TreeGrid::ClearNodes(PNode N)							{ if (N) N->nodes.clear(); }
void TreeGrid::SetFocusNode(PNode N, bool b)				{ if (N) { tree.FocusNode(N, b); RefreshTreeGrid(); tree.SetFocus(); }}
PNode TreeGrid::GetFocusNode()								{ return tree.pCurNode; }
size_t TreeGrid::GetFocusCell()								{ return tree.cur_column; }
PNode TreeGrid::GetPrevNode(PNode N)						{ return tree.prev_node(N); }
PNode TreeGrid::GetNextNode(PNode N)						{ return tree.next_node(N); }
//bool TreeGrid::NodeHasCell(PNode N, size_t cellidx)			{ return ((N)?cellidx<N->CellCount():false); }
Cell& TreeGrid::GetCell(PNode N, size_t cellidx)			{ if (N) return N->CellAt(cellidx); throw std::logic_error("TreeGrid::GetCell(<nullptr!>)"); }

void TreeGrid::OnLeftDown(PNode N, dword flags)
{
	if (N)
	{
		if (flags&K_SHIFT) { tree.pSelPivot=tree.pCurNode; SelectRange(tree.pCurNode, N); }
		else if (flags&K_CTRL) { tree.pSelPivot=nullptr; Select(N, !N->IsSelected()); }
		else ClearSelection();
		SetFocusNode(N);
	}
}

void TreeGrid::OnLeftDouble(PNode N, size_t cellidx)
{
	if (N)
	{
		ClearSelection();
		tree.pSelPivot=nullptr;
		SetFocusNode(N);
		WhenNodeCellActivate(N, cellidx);
	}
}

void TreeGrid::FillMenuBar(MenuBar &menubar)
{
//	if (b_add_tg_help)
//	{
//		menubar.Add("TreeGrid Help", THISBACK(OnTGHelp));
//		menubar.Separator();
//	}
	WhenMenuBar(menubar);
}

//void TreeGrid::OnTGHelp()
//{
//	return; //no unexpected behaviour
//	std::string s = "\nMouse:\n"
//					"     o  (normal stuff)\n"
//					"     o  Right-click : context popup menu\n"
//					"     o  Ctrl+Left-click : multiple row select\n"
//					"\nKeyboard:\n"
//					"     o  Left/Right arrow-keys : selects along the cells in a row\n"
//					"     o  Ctrl+Left/Right arrow-keys : expand/contract current node (if it can)\n"
//					"     o  Ctrl+M : context popup menu\n"
//					"     o  Up/Down, PageUp/PageDown : (as expected)\n"
//					"     o  Shift+Up/Down arrow-keys : multiple row select\n"
//					"     o  Home/End : first/last cell in row\n"
//					"     o  Ctrl+Home/End : top and bottom of tree\n"
//					"\nUnder construction (feedback appreciated)\n";
//					//"     - To Do:\n"
//					//"          o  make rows variable height and text-nodes wrapable\n"
//					//"          o  adding controls to cells/columns\n"
//					//"          o  ...\n";
//
//	PromptOK(DeQtf(s.c_str()));
//}

void TreeGrid::OnRightDown(PNode)// N)
{
	MenuBar menubar;
	FillMenuBar(menubar);
	menubar.Execute();
}

void TreeGrid::ShowPopMenu(Point p)
{
	MenuBar menubar;
	FillMenuBar(menubar);
	if (!menubar.IsEmpty())	menubar.Execute(this, p);
}

void TreeGrid::ListExpandingNodes(EXDisp exd)				{ tree.exd=exd; SyncNodes(); }
EXDisp TreeGrid::ListExpandingNodes()						{ return tree.exd; }
void TreeGrid::ShowTreeLines(bool bShow)					{ tree.bShowTreeLines=bShow; Refresh(); }
bool TreeGrid::ShowTreeLines()								{ return tree.bShowTreeLines; }

void TreeGrid::ShowHeader(bool bShow)
{
	header.show(bShow);
	if (!bShow) tree.SetRectY(0, tree.GetSize().cy+G_LINE_HEIGHT);
	else { tree.SetRectY(G_LINE_HEIGHT+1, tree.GetSize().cy-G_LINE_HEIGHT-1); }
	Layout();
	Refresh();
}

bool TreeGrid::ShowHeader()									{ return header.show(); }

void TreeGrid::UseTGHelpMenu(bool b)						{ b_add_tg_help=b; }

