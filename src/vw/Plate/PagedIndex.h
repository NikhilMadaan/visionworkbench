// __BEGIN_LICENSE__
// Copyright (C) 2006-2009 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__
#ifndef __VW_PLATEFILE_PAGED_INDEX_H__
#define __VW_PLATEFILE_PAGED_INDEX_H__

#include <vw/Core/FundamentalTypes.h>
#include <vw/Core/Cache.h>

#include <vw/Plate/Index.h>
#include <vw/Plate/IndexPage.h>
#include <vw/Plate/BlobManager.h>
#include <vw/Plate/ProtoBuffers.pb.h>

#include <google/sparsetable>

#include <vector>
#include <string>
#include <list>

namespace vw {
namespace platefile {

  // --------------------------------------------------------------------
  //                             INDEX LEVEL
  // --------------------------------------------------------------------
  class IndexLevel {

    int m_level;
    int m_page_width, m_page_height;
    int m_horizontal_pages, m_vertical_pages;
    std::vector<boost::shared_ptr<IndexPageGenerator> > m_cache_generators;
    std::vector<Cache::Handle<IndexPageGenerator> > m_cache_handles;
    vw::Cache m_cache;

  public:
    typedef IndexPage::multi_value_type multi_value_type;

    IndexLevel(boost::shared_ptr<PageGeneratorFactory> page_gen_factory,
               int level, int page_width, int page_height, int cache_size);

    ~IndexLevel();

    /// Sync any unsaved data in the index to disk.
    void sync();

    /// Fetch the value of an index node at this level.
    IndexRecord get(int32 col, int32 row, int32 transaction_id, bool exact_match = false) const;

    /// Fetch the value of an index node at this level.
    multi_value_type multi_get(int32 col, int32 row, 
                               int32 begin_transaction_id, 
                               int32 end_transaction_id) const; 

    /// Set the value of an index node at this level.
    void set(TileHeader const& hdr, IndexRecord const& rec);

    /// Returns a list of valid tiles at this level.
    std::list<TileHeader> valid_tiles(BBox2i const& region,
                                      int start_transaction_id, 
                                      int end_transaction_id, 
                                      int min_num_matches) const;
  };

  // --------------------------------------------------------------------
  //                             PAGED INDEX
  // --------------------------------------------------------------------

  class PagedIndex : public Index {
    boost::shared_ptr<PageGeneratorFactory> m_page_gen_factory;

  protected:

    std::vector<boost::shared_ptr<IndexLevel> > m_levels;
    int m_page_width, m_page_height;
    int m_default_cache_size;

    virtual void commit_record(TileHeader const& header, IndexRecord const& record) = 0;

  public:
    typedef IndexLevel::multi_value_type multi_value_type;

    /// Create a new, empty index.
    PagedIndex(boost::shared_ptr<PageGeneratorFactory> page_generator,
               IndexHeader new_index_info, 
               int page_width = 256, int page_height = 256, 
               int default_cache_size = 10000);

    /// Open an existing index from a file on disk.
    PagedIndex(boost::shared_ptr<PageGeneratorFactory> page_generator,
               int page_width = 256, int page_height = 256, 
               int default_cache_size = 10000);

    virtual ~PagedIndex() {}

    /// Sync any unsaved data in the index to disk.
    virtual void sync();

    // ----------------------- READ/WRITE REQUESTS  ----------------------

    /// Attempt to access a tile in the index.  Throws an
    /// TileNotFoundErr if the tile cannot be found.
    /// 
    /// By default, this call to read will return a tile with the MOST
    /// RECENT transaction_id <= to the transaction_id you specify
    /// here in the function arguments (if a tile exists).  However,
    /// setting exact_transaction_match = true will force the
    /// PlateFile to search for a tile that has the EXACT SAME
    /// transaction_id as the one that you specify.
    ///
    /// A transaction ID of -1 indicates that we should return the
    /// most recent tile, regardless of its transaction id.
    virtual IndexRecord read_request(int col, int row, int level, 
                                     int transaction_id, bool exact_transaction_match = false);

    /// Return multiple index entries that match the specified
    /// transaction id range.  This range is inclusive of the first
    /// entry, but not the last entry: [ begin_transaction_id, end_transaction_id )
    ///
    /// Results are return as a std::pair<int32, IndexRecord>.  The
    /// first value in the pair is the transaction id for that
    /// IndexRecord.
    virtual std::list<std::pair<int32, IndexRecord> > multi_read_request(int col, int row, int level, 
                                                                         int begin_transaction_id, 
                                                                         int end_transaction_id);

    // Writing, pt. 1: Locks a blob and returns the blob id that can
    // be used to write a tile.
    virtual int write_request(int size) = 0;

    // Writing, pt. 2: Supply information to update the index and
    // unlock the blob id.
    virtual void write_update(TileHeader const& header, IndexRecord const& record);
  
    /// Writing, pt. 3: Signal the completion 
    virtual void write_complete(int blob_id, uint64 blob_offset) = 0;

    // ----------------------- PROPERTIES  ----------------------

    /// Returns a list of valid tiles that match this level, region, and
    /// range of transaction_id's.  Returns a list of TileHeaders with
    /// col/row/level and transaction_id of the most recent tile at each
    /// valid location.  Note: there may be other tiles in the transaction
    /// range at this col/row/level, but valid_tiles() only returns the
    /// first one.
    virtual std::list<TileHeader> valid_tiles(int level, BBox2i const& region,
                                              int start_transaction_id,
                                              int end_transaction_id,
                                              int min_num_matches) const;
  };

}} // namespace vw::platefile

#endif // __VW_PLATEFILE_PAGED_INDEX_H__
