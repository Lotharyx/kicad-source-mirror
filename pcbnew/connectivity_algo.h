/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// #define CONNECTIVITY_DEBUG

#ifndef __CONNECTIVITY_ALGO_H
#define __CONNECTIVITY_ALGO_H

#include <class_board.h>
#include <class_pad.h>
#include <class_module.h>
#include <class_zone.h>

#include <geometry/shape_poly_set.h>
#include <geometry/poly_grid_partition.h>

#include <memory>
#include <algorithm>
#include <functional>
#include <vector>
#include <deque>
#include <intrusive_list.h>

#include <connectivity_rtree.h>
#include <connectivity_data.h>

class CN_ITEM;
class CN_CONNECTIVITY_ALGO_IMPL;
class CN_RATSNEST_NODES;
class CN_CLUSTER;
class BOARD;
class BOARD_CONNECTED_ITEM;
class BOARD_ITEM;
class ZONE_CONTAINER;
class PROGRESS_REPORTER;

class CN_ANCHOR
{
public:
    CN_ANCHOR()
    {
        m_item = nullptr;
    }

    CN_ANCHOR( const VECTOR2I& aPos, CN_ITEM* aItem )
    {
        m_pos   = aPos;
        m_item  = aItem;
        assert( m_item );
    }

    bool Valid() const;


    CN_ITEM* Item() const
    {
        return m_item;
    }

    BOARD_CONNECTED_ITEM* Parent() const;

    const VECTOR2I& Pos() const
    {
        return m_pos;
    }

    bool IsDirty() const;

    /// Returns tag, common identifier for connected nodes
    inline int GetTag() const
    {
        return m_tag;
    }

    /// Sets tag, common identifier for connected nodes
    inline void SetTag( int aTag )
    {
        m_tag = aTag;
    }

    /// Decides whether this node can be a ratsnest line target
    inline void SetNoLine( bool aEnable )
    {
        m_noline = aEnable;
    }

    /// Returns true if this node can be a target for ratsnest lines
    inline const bool& GetNoLine() const
    {
        return m_noline;
    }

    inline void SetCluster( std::shared_ptr<CN_CLUSTER> aCluster )
    {
        m_cluster = aCluster;
    }

    inline std::shared_ptr<CN_CLUSTER> GetCluster() const
    {
        return m_cluster;
    }

    /**
     * has meaning only for tracks and vias.
     * @return true if this anchor is dangling
     * The anchor point is dangling if the parent is a track
     * and this anchor point is not connected to another item
     * ( track, vas pad or zone) or if the parent is a via and this anchor point
     * is connected to only one track and not to another item
     */
    bool IsDangling() const;

    // Tag used for unconnected items.
    static const int TAG_UNCONNECTED = -1;

private:
    /// Position of the anchor
    VECTOR2I m_pos;

    /// Item owning the anchor
    CN_ITEM* m_item = nullptr;

    /// Tag for quick connection resolution
    int m_tag = -1;

    /// Whether it the node can be a target for ratsnest lines
    bool m_noline = false;

    /// Cluster to which the anchor belongs
    std::shared_ptr<CN_CLUSTER> m_cluster;
};


typedef std::shared_ptr<CN_ANCHOR>  CN_ANCHOR_PTR;
typedef std::vector<CN_ANCHOR_PTR>  CN_ANCHORS;


class CN_EDGE
{
public:
    CN_EDGE() {};
    CN_EDGE( CN_ANCHOR_PTR aSource, CN_ANCHOR_PTR aTarget, int aWeight = 0 ) :
        m_source( aSource ),
        m_target( aTarget ),
        m_weight( aWeight ) {}

    CN_ANCHOR_PTR GetSourceNode() const { return m_source; }
    CN_ANCHOR_PTR GetTargetNode() const { return m_target; }
    int GetWeight() const { return m_weight; }

    void SetSourceNode( const CN_ANCHOR_PTR& aNode ) { m_source = aNode; }
    void SetTargetNode( const CN_ANCHOR_PTR& aNode ) { m_target = aNode; }
    void SetWeight( unsigned int weight ) { m_weight = weight; }

    void SetVisible( bool aVisible )
    {
        m_visible = aVisible;
    }

    bool IsVisible() const
    {
        return m_visible;
    }

    const VECTOR2I GetSourcePos() const
    {
        return m_source->Pos();
    }

    const VECTOR2I GetTargetPos() const
    {
        return m_target->Pos();
    }

private:
    CN_ANCHOR_PTR m_source;
    CN_ANCHOR_PTR m_target;
    unsigned int m_weight = 0;
    bool m_visible = true;
};


class CN_CLUSTER
{
private:

    bool m_conflicting = false;
    int m_originNet = 0;
    CN_ITEM* m_originPad = nullptr;
    std::vector<CN_ITEM*> m_items;

public:
    CN_CLUSTER();
    ~CN_CLUSTER();

    bool HasValidNet() const
    {
        return m_originNet >= 0;
    }

    int OriginNet() const
    {
        return m_originNet;
    }

    wxString OriginNetName() const;

    bool    Contains( const CN_ITEM* aItem );
    bool    Contains( const BOARD_CONNECTED_ITEM* aItem );
    void    Dump();

    int Size() const
    {
        return m_items.size();
    }

    bool HasNet() const
    {
        return m_originNet >= 0;
    }

    bool IsOrphaned() const
    {
        return m_originPad == nullptr;
    }

    bool IsConflicting() const
    {
        return m_conflicting;
    }

    void Add( CN_ITEM* item );

    using ITER = decltype(m_items)::iterator;

    ITER begin() { return m_items.begin(); };
    ITER end() { return m_items.end(); };
};

typedef std::shared_ptr<CN_CLUSTER> CN_CLUSTER_PTR;


// basic connectivity item
class CN_ITEM : public INTRUSIVE_LIST<CN_ITEM>
{
private:
    BOARD_CONNECTED_ITEM* m_parent;

    using CONNECTED_ITEMS = std::vector<CN_ITEM*>;

    ///> list of items physically connected (touching)
    CONNECTED_ITEMS m_connected;

    CN_ANCHORS m_anchors;

    ///> visited flag for the BFS scan
    bool m_visited;

    ///> can the net propagator modify the netcode?
    bool m_canChangeNet;

    ///> valid flag, used to identify garbage items (we use lazy removal)
    bool m_valid;

protected:
    ///> dirty flag, used to identify recently added item not yet scanned into the connectivity search
    bool m_dirty;

    ///> layer range over which the item exists
    LAYER_RANGE m_layers;

    ///> bounding box for the item
    BOX2I m_bbox;

public:
    void Dump();

    CN_ITEM( BOARD_CONNECTED_ITEM* aParent, bool aCanChangeNet, int aAnchorCount = 2 )
    {
        m_parent = aParent;
        m_canChangeNet = aCanChangeNet;
        m_visited = false;
        m_valid = true;
        m_dirty = true;
        m_anchors.reserve( 2 );
        m_layers = LAYER_RANGE( 0, PCB_LAYER_ID_COUNT );
    }

    virtual ~CN_ITEM() {};

    void AddAnchor( const VECTOR2I& aPos )
    {
        m_anchors.emplace_back( std::make_unique<CN_ANCHOR>( aPos, this ) );
    }

    CN_ANCHORS& Anchors()
    {
        return m_anchors;
    }

    void SetValid( bool aValid )
    {
        m_valid = aValid;
    }

    bool Valid() const
    {
        return m_valid;
    }

    void SetDirty( bool aDirty )
    {
        m_dirty = aDirty;
    }

    bool Dirty() const
    {
        return m_dirty;
    }

    /**
     * Function SetLayers()
     *
     * Sets the layers spanned by the item to aLayers.
     */
    void SetLayers( const LAYER_RANGE& aLayers )
    {
        m_layers = aLayers;
    }

    /**
     * Function SetLayer()
     *
     * Sets the layers spanned by the item to a single layer aLayer.
     */
    void SetLayer( int aLayer )
    {
        m_layers = LAYER_RANGE( aLayer, aLayer );
    }

    /**
     * Function Layers()
     *
     * Returns the contiguous set of layers spanned by the item.
     */
    const LAYER_RANGE& Layers() const
    {
        return m_layers;
    }

    /**
     * Function Layer()
     *
     * Returns the item's layer, for single-layered items only.
     */
    virtual int Layer() const
    {
        return Layers().Start();
    }

    /**
     * Function LayersOverlap()
     *
     * Returns true if the set of layers spanned by aOther overlaps our
     * layers.
     */
    bool LayersOverlap( const CN_ITEM* aOther ) const
    {
        return Layers().Overlaps( aOther->Layers() );
    }

    const BOX2I& BBox()
    {
        if( m_dirty )
        {
            EDA_RECT box = m_parent->GetBoundingBox();
            m_bbox = BOX2I( box.GetPosition(), box.GetSize() );
        }
        return m_bbox;
    }

    BOARD_CONNECTED_ITEM* Parent() const
    {
        return m_parent;
    }

    const CONNECTED_ITEMS& ConnectedItems()  const
    {
        return m_connected;
    }

    void ClearConnections()
    {
        m_connected.clear();
    }

    void SetVisited( bool aVisited )
    {
        m_visited = aVisited;
    }

    bool Visited() const
    {
        return m_visited;
    }

    bool CanChangeNet() const
    {
        return m_canChangeNet;
    }

    static void Connect( CN_ITEM* a, CN_ITEM* b )
    {
        bool foundA = false, foundB = false;

        for( auto item : a->m_connected )
        {
            if( item == b )
            {
                foundA = true;
                break;
            }
        }

        for( auto item : b->m_connected )
        {
            if( item == a )
            {
                foundB = true;
                break;
            }
        }

        if( !foundA )
            a->m_connected.push_back( b );

        if( !foundB )
            b->m_connected.push_back( a );
    }

    void RemoveInvalidRefs();

    virtual int             AnchorCount() const;
    virtual const VECTOR2I  GetAnchor( int n ) const;

    int Net() const;
};

typedef std::shared_ptr<CN_ITEM> CN_ITEM_PTR;


class CN_LIST
{
private:
    bool m_dirty;
    bool m_hasInvalid;

    CN_RTREE<CN_ITEM*> m_index;

protected:
    std::vector<CN_ITEM*> m_items;

    void addItemtoTree( CN_ITEM* item )
    {
        m_index.Insert( item );
    }

public:
    CN_LIST()
    {
        m_dirty = false;
        m_hasInvalid = false;
    }

    void Clear()
    {
        for( auto item : m_items )
            delete item;

        m_items.clear();
        m_index.RemoveAll();
    }

    using ITER = decltype(m_items)::iterator;

    ITER begin() { return m_items.begin(); };
    ITER end() { return m_items.end(); };

    CN_ITEM* operator[] ( int aIndex ) { return m_items[aIndex]; }

    template <class T>
    void FindNearby( CN_ITEM *aItem, T aFunc );

    void SetHasInvalid( bool aInvalid = true )
    {
        m_hasInvalid = aInvalid;
    }

    void SetDirty( bool aDirty = true )
    {
        m_dirty = aDirty;
    }

    bool IsDirty() const
    {
        return m_dirty;
    }

    void RemoveInvalidItems( std::vector<CN_ITEM*>& aGarbage );

    void ClearDirtyFlags()
    {
        for( auto item : m_items )
            item->SetDirty( false );

        SetDirty( false );
    }

    void MarkAllAsDirty()
    {
        for( auto item : m_items )
            item->SetDirty( true );

        SetDirty( true );
    }

    int Size() const
    {
        return m_items.size();
    }

    CN_ITEM* Add( D_PAD* pad )
    {
        auto item = new CN_ITEM( pad, false, 1 );
        item->AddAnchor( pad->ShapePos() );
        item->SetLayers( LAYER_RANGE( 0, PCB_LAYER_ID_COUNT ) );

        switch( pad->GetAttribute() )
        {
        case PAD_ATTRIB_SMD:
        case PAD_ATTRIB_HOLE_NOT_PLATED:
        case PAD_ATTRIB_CONN:
        {
            LSET lmsk = pad->GetLayerSet();

            for( int i = 0; i <= MAX_CU_LAYERS; i++ )
            {
                if( lmsk[i] )
                {
                    item->SetLayer( i );
                    break;
                }
            }
        }
        default:
            break;
        }

        addItemtoTree( item );
        m_items.push_back( item );
        SetDirty();
        return item;
    }

    CN_ITEM* Add( TRACK* track )
    {
        auto item = new CN_ITEM( track, true );
        m_items.push_back( item );
        item->AddAnchor( track->GetStart() );
        item->AddAnchor( track->GetEnd() );
        item->SetLayer( track->GetLayer() );
        addItemtoTree( item );
        SetDirty();
        return item;
    }

    CN_ITEM* Add( VIA* via )
    {
        auto item = new CN_ITEM( via, true, 1 );

        m_items.push_back( item );
        item->AddAnchor( via->GetStart() );
        item->SetLayers( LAYER_RANGE( 0, PCB_LAYER_ID_COUNT ) );
        addItemtoTree( item );
        SetDirty();
        return item;
    }
};


class CN_ZONE : public CN_ITEM
{
public:
    CN_ZONE( ZONE_CONTAINER* aParent, bool aCanChangeNet, int aSubpolyIndex ) :
        CN_ITEM( aParent, aCanChangeNet ),
        m_subpolyIndex( aSubpolyIndex )
    {
        SHAPE_LINE_CHAIN outline = aParent->GetFilledPolysList().COutline( aSubpolyIndex );

        outline.SetClosed( true );
        outline.Simplify();

        m_cachedPoly.reset( new POLY_GRID_PARTITION( outline, 16 ) );
    }

    int SubpolyIndex() const
    {
        return m_subpolyIndex;
    }

    bool ContainsAnchor( const CN_ANCHOR_PTR anchor ) const
    {
        return ContainsPoint( anchor->Pos() );
    }

    bool ContainsPoint( const VECTOR2I p ) const
    {
        auto zone = static_cast<ZONE_CONTAINER*> ( Parent() );
        return m_cachedPoly->ContainsPoint( p, zone->GetMinThickness() );
    }

    const BOX2I& BBox()
    {
        if( m_dirty )
            m_bbox = m_cachedPoly->BBox();

        return m_bbox;
    }

    virtual int             AnchorCount() const override;
    virtual const VECTOR2I  GetAnchor( int n ) const override;

private:
    std::vector<VECTOR2I> m_testOutlinePoints;
    std::unique_ptr<POLY_GRID_PARTITION> m_cachedPoly;
    int m_subpolyIndex;
};


class CN_ZONE_LIST : public CN_LIST
{
public:
    CN_ZONE_LIST() {}

    const std::vector<CN_ITEM*> Add( ZONE_CONTAINER* zone )
    {
        const auto& polys = zone->GetFilledPolysList();

        std::vector<CN_ITEM*> rv;

        for( int j = 0; j < polys.OutlineCount(); j++ )
        {
            CN_ZONE* zitem = new CN_ZONE( zone, false, j );
            const auto& outline = zone->GetFilledPolysList().COutline( j );

            for( int k = 0; k < outline.PointCount(); k++ )
                zitem->AddAnchor( outline.CPoint( k ) );

            m_items.push_back( zitem );
            zitem->SetLayer( zone->GetLayer() );
            addItemtoTree( zitem );
            rv.push_back( zitem );
            SetDirty();
        }

        return rv;
    }
};

template <class T>
void CN_LIST::FindNearby( CN_ITEM *aItem, T aFunc )
{
    m_index.Query( aItem->BBox(), aItem->Layers(), aFunc );
}

class CN_CONNECTIVITY_ALGO
{
public:
    enum CLUSTER_SEARCH_MODE
    {
        CSM_PROPAGATE,
        CSM_CONNECTIVITY_CHECK,
        CSM_RATSNEST
    };

    using CLUSTERS = std::vector<CN_CLUSTER_PTR>;

private:

    class ITEM_MAP_ENTRY
    {
public:
        ITEM_MAP_ENTRY( CN_ITEM* aItem = nullptr )
        {
            if( aItem )
                m_items.push_back( aItem );
        }

        void MarkItemsAsInvalid()
        {
            for( auto item : m_items )
            {
                item->SetValid( false );
            }
        }

        void Link( CN_ITEM* aItem )
        {
            m_items.push_back( aItem );
        }

        const std::list<CN_ITEM*> GetItems() const
        {
            return m_items;
        }

        std::list<CN_ITEM*> m_items;
    };

    std::mutex m_listLock;
    CN_LIST m_itemList;
    CN_ZONE_LIST m_zoneList;

    using ITEM_MAP_PAIR = std::pair <const BOARD_CONNECTED_ITEM*, ITEM_MAP_ENTRY>;

    std::unordered_map<const BOARD_CONNECTED_ITEM*, ITEM_MAP_ENTRY> m_itemMap;

    CLUSTERS m_connClusters;
    CLUSTERS m_ratsnestClusters;
    std::vector<bool> m_dirtyNets;
    PROGRESS_REPORTER* m_progressReporter = nullptr;

    void    searchConnections();

    void    update();
    void    propagateConnections();

    template <class Container, class BItem>
    void add( Container& c, BItem brditem )
    {
        auto item = c.Add( brditem );

        m_itemMap[ brditem ] = ITEM_MAP_ENTRY( item );
    }

    bool addConnectedItem( BOARD_CONNECTED_ITEM* aItem );
    bool isDirty() const;

    void markItemNetAsDirty( const BOARD_ITEM* aItem );

public:

    CN_CONNECTIVITY_ALGO();
    ~CN_CONNECTIVITY_ALGO();

    bool ItemExists( const BOARD_CONNECTED_ITEM* aItem )
    {
        return m_itemMap.find( aItem ) != m_itemMap.end();
    }

    ITEM_MAP_ENTRY& ItemEntry( const BOARD_CONNECTED_ITEM* aItem )
    {
        return m_itemMap[ aItem ];
    }

    bool IsNetDirty( int aNet ) const
    {
        if( aNet < 0 )
            return false;

        return m_dirtyNets[ aNet ];
    }

    void ClearDirtyFlags()
    {
        for( auto i = m_dirtyNets.begin(); i != m_dirtyNets.end(); ++i )
            *i = false;
    }

    void GetDirtyClusters( CLUSTERS& aClusters )
    {
        for( auto cl : m_ratsnestClusters )
        {
            int net = cl->OriginNet();

            if( net >= 0 && m_dirtyNets[net] )
                aClusters.push_back( cl );
        }
    }

    int NetCount() const
    {
        return m_dirtyNets.size();
    }

    void    Build( BOARD* aBoard );
    void    Build( const std::vector<BOARD_ITEM*>& aItems );

    void Clear();

    bool    Remove( BOARD_ITEM* aItem );
    bool    Add( BOARD_ITEM* aItem );

    const CLUSTERS  SearchClusters( CLUSTER_SEARCH_MODE aMode, const KICAD_T aTypes[], int aSingleNet );
    const CLUSTERS  SearchClusters( CLUSTER_SEARCH_MODE aMode );

    void    PropagateNets();
    void    FindIsolatedCopperIslands( ZONE_CONTAINER* aZone, std::vector<int>& aIslands );

    /**
     * Finds the copper islands that are not connected to a net.  These are added to
     * the m_islands vector.
     * N.B. This must be called after aZones has been refreshed.
     * @param: aZones The set of zones to search for islands
     */
    void    FindIsolatedCopperIslands( std::vector<CN_ZONE_ISOLATED_ISLAND_LIST>& aZones );

    bool    CheckConnectivity( std::vector<CN_DISJOINT_NET_ENTRY>& aReport );

    const CLUSTERS& GetClusters();
    int             GetUnconnectedCount();

    CN_LIST& ItemList() { return m_itemList; }

    void ForEachAnchor( const std::function<void( CN_ANCHOR& )>& aFunc );
    void ForEachItem( const std::function<void( CN_ITEM& )>& aFunc );

    void MarkNetAsDirty( int aNet );
    void SetProgressReporter( PROGRESS_REPORTER* aReporter );

};

bool operator<( const CN_ANCHOR_PTR& a, const CN_ANCHOR_PTR& b );


/**
 * Struct CN_VISTOR
 **/
class CN_VISITOR {

public:

    CN_VISITOR( CN_ITEM* aItem, std::mutex* aListLock ) :
        m_item( aItem ),
        m_listLock( aListLock )
    {}

    bool operator()( CN_ITEM* aCandidate );

protected:

    void checkZoneItemConnection( CN_ZONE* aZone, CN_ITEM* aItem );

    void checkZoneZoneConnection( CN_ZONE* aZoneA, CN_ZONE* aZoneB );

    ///> the item we are looking for connections to
    CN_ITEM* m_item;

    ///> the mutex protecting our connection list
    std::mutex* m_listLock;

};

#endif
