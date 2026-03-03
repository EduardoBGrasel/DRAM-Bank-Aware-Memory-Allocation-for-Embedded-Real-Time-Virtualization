#include "../inc/mser.h"

// Referências externas aos buffers globais definidos no seu main.c
extern unsigned char *sdims;
extern unsigned char *ssubs_pt;
extern unsigned char *snsubs_pt;
extern unsigned char *sstrides_pt;
extern unsigned char *svisited_pt;
extern unsigned char *sjoins_pt;
extern unsigned char *sforest_pt;
extern unsigned char *sregion_pt;
extern unsigned char *spairs_pt;
extern unsigned char *sacc_pt;
extern unsigned char *sell_pt;
extern unsigned char *spt;

/* advance N-dimensional subscript */
void adv(iArray *dims, int ndims, iArray *subs_pt) {
  int d = 0;
  while (d < ndims) {
    sref(subs_pt, d) = sref(subs_pt, d) + 1;
    if (sref(subs_pt, d) < sref(dims, d))
      return;
    sref(subs_pt, d++) = 0;
  }
}

I2D *mser(I2D *I, int in_delta) {
  idx_t i, rindex = 0;
  int k;
  int nout = 1;
  int BUCKETS = 256;

  I2D *out;

  /* configuration */
  int small_cleanup = 1; 
  int big_cleanup = 1;   
  int bad_cleanup = 0;   
  int dup_cleanup = 1;   
  val_t delta;           

  /* node value denoting a void node */
  idx_t const node_is_void = 0xffffffff;

  iArray *subs_pt;     
  iArray *nsubs_pt;    
  uiArray *strides_pt; 
  uiArray *visited_pt; 

  int nel;        
  int ner = 0;    
  int nmer = 0;   
  int ndims;      
  iArray *dims;   
  int njoins = 0; 

  I2D *I_pt;            
  pair_t *pairs_pt;     
  node_t *forest_pt;    
  region_t *regions_pt; 

  /* ellipses fitting */
  ulliArray *acc_pt; 
  ulliArray *ell_pt; 
  int gdl;           
  uiArray *joins_pt; 

  delta = in_delta;

  /* Inicialização dos ponteiros para os buffers estáticos */
  nel = I->height * I->width; 
  ndims = 2;
  
  dims       = (iArray *)sdims;
  subs_pt    = (iArray *)ssubs_pt;
  nsubs_pt   = (iArray *)snsubs_pt;
  strides_pt = (uiArray *)sstrides_pt;
  visited_pt = (uiArray *)svisited_pt;
  joins_pt   = (uiArray *)sjoins_pt;
  regions_pt = (region_t *)sregion_pt;
  pairs_pt   = (pair_t *)spairs_pt;
  forest_pt  = (node_t *)sforest_pt;
  acc_pt     = (ulliArray *)sacc_pt;
  ell_pt     = (ulliArray *)sell_pt;

  I_pt = I;

  sref(dims, 0) = I->height;
  sref(dims, 1) = I->width;

  /* compute strides to move into the N-dimensional image array */
  sref(strides_pt, 0) = 1;
  for (k = 1; k < ndims; ++k) {
    sref(strides_pt, k) = sref(strides_pt, k - 1) * sref(dims, k - 1);
  }

  /* sort pixels in increasing order of intensity: using Bucket Sort */
  {
    unsigned int buckets[256];
    memset(buckets, 0, sizeof(unsigned int) * BUCKETS);

    for (i = 0; i < nel; ++i) {
      val_t v = asubsref(I_pt, i);
      ++buckets[v];
    }

    for (i = 1; i < BUCKETS; ++i) {
      arrayref(buckets, i) += arrayref(buckets, i - 1);
    }

    for (i = nel; i >= 1;) {
      val_t v = asubsref(I_pt, --i);
      idx_t j = --buckets[v];
      pairs_pt[j].value = v;
      pairs_pt[j].index = i;
    }
  }

  /* initialize the forest with all void nodes */
  for (i = 0; i < nel; ++i) {
    forest_pt[i].parent = node_is_void;
  }

  /* number of ellipse free parameters */
  gdl = ndims * (ndims + 1) / 2 + ndims;

  /* Compute extremal regions tree */
  for (i = 0; i < nel; ++i) {
    idx_t index = pairs_pt[i].index;
    val_t value = pairs_pt[i].value;
    rindex = index;

    forest_pt[index].parent = index;
    forest_pt[index].shortcut = index;
    forest_pt[index].area = 1;

    {
      idx_t temp = index;
      for (k = ndims - 1; k >= 0; --k) {
        sref(nsubs_pt, k) = -1;
        sref(subs_pt, k) = temp / sref(strides_pt, k);
        temp = temp % sref(strides_pt, k);
      }
    }

    while (1) {
      int good = 1;
      idx_t nindex = 0;

      for (k = 0; k < ndims && good; ++k) {
        int temp = sref(nsubs_pt, k) + sref(subs_pt, k);
        good &= 0 <= temp && temp < sref(dims, k);
        nindex += temp * sref(strides_pt, k);
      }

      if (good && nindex != index && forest_pt[nindex].parent != node_is_void) {
        idx_t nrindex = 0, nvisited;
        val_t nrvalue = 0;

        nvisited = 0;
        while (forest_pt[rindex].shortcut != rindex) {
          sref(visited_pt, nvisited++) = rindex;
          rindex = forest_pt[rindex].shortcut;
        }
        while (nvisited--) {
          forest_pt[sref(visited_pt, nvisited)].shortcut = rindex;
        }

        nrindex = nindex;
        nvisited = 0;
        while (forest_pt[nrindex].shortcut != nrindex) {
          sref(visited_pt, nvisited++) = nrindex;
          nrindex = forest_pt[nrindex].shortcut;
        }
        while (nvisited--) {
          forest_pt[sref(visited_pt, nvisited)].shortcut = nrindex;
        }

        if (rindex != nrindex) {
          nrvalue = asubsref(I_pt, nrindex);
          if (nrvalue == value) {
            forest_pt[rindex].parent = nrindex;
            forest_pt[rindex].shortcut = nrindex;
            forest_pt[nrindex].area += forest_pt[rindex].area;
            sref(joins_pt, njoins++) = rindex;
          } else {
            forest_pt[nrindex].parent = rindex;
            forest_pt[nrindex].shortcut = rindex;
            forest_pt[rindex].area += forest_pt[nrindex].area;

            if (nrvalue != value) {
              forest_pt[nrindex].region = ner;
              regions_pt[ner].index = nrindex;
              regions_pt[ner].parent = ner;
              regions_pt[ner].value = nrvalue;
              regions_pt[ner].area = forest_pt[nrindex].area;
              regions_pt[ner].area_top = nel;
              regions_pt[ner].area_bot = 0;
              ++ner;
            }
            sref(joins_pt, njoins++) = nrindex;
          }
        }
      }

      k = 0;
      sref(nsubs_pt, k) = sref(nsubs_pt, k) + 1;
      while (sref(nsubs_pt, k) > 1) {
        sref(nsubs_pt, k++) = -1;
        if (k == ndims)
          goto done_all_neighbors;
        sref(nsubs_pt, k) = sref(nsubs_pt, k) + 1;
      }
    } 
  done_all_neighbors:;
  } 

  forest_pt[rindex].region = ner;
  regions_pt[ner].index = rindex;
  regions_pt[ner].parent = ner;
  regions_pt[ner].value = asubsref(I_pt, rindex);
  regions_pt[ner].area = forest_pt[rindex].area;
  regions_pt[ner].area_top = nel;
  regions_pt[ner].area_bot = 0;
  ++ner;

  /* Compute region parents */
  for (i = 0; i < ner; ++i) {
    idx_t index = regions_pt[i].index;
    val_t value = regions_pt[i].value;
    idx_t j = i;
    while (j == i) {
      idx_t pindex = forest_pt[index].parent;
      val_t pvalue = asubsref(I_pt, pindex);
      if (index == pindex) {
        j = forest_pt[index].region;
        break;
      }
      if (value < pvalue) {
        j = forest_pt[index].region;
      }
      index = pindex;
      value = pvalue;
    }
    regions_pt[i].parent = j;
  }

  /* Compute areas of tops and bottoms */
  for (i = 0; i < ner; ++i) {
    idx_t parent = regions_pt[i].parent;
    int val0 = regions_pt[i].value;
    int val1 = regions_pt[parent].value;
    int val = val0;
    idx_t j = i;
    while (1) {
      int valp = regions_pt[parent].value;
      if (val0 <= val - delta && val - delta < val1) {
        regions_pt[j].area_bot = MAX(regions_pt[j].area_bot, regions_pt[i].area);
      }
      if (val <= val0 + delta && val0 + delta < valp) {
        regions_pt[i].area_top = regions_pt[j].area;
      }
      if (val1 <= val - delta && val0 + delta < val) break;
      if (j == parent) break;
      j = parent;
      parent = regions_pt[j].parent;
      val = valp;
    }
  }

  /* Compute variation */
  for (i = 0; i < ner; ++i) {
    int area = regions_pt[i].area;
    int area_top = regions_pt[i].area_top;
    int area_bot = regions_pt[i].area_bot;
    regions_pt[i].variation = (area_top - area_bot) / (int)(area * 1);
    regions_pt[i].maxstable = 1;
  }

  /* Remove regions which are NOT maximally stable */
  nmer = ner;
  for (i = 0; i < ner; ++i) {
    idx_t parent = regions_pt[i].parent;
    int var = regions_pt[i].variation;       
    int pvar = regions_pt[parent].variation; 
    idx_t loser;
    if (var < pvar) loser = parent;
    else loser = i;
    if (regions_pt[loser].maxstable) --nmer;
    regions_pt[loser].maxstable = 0;
  }

  /* Cleanup */
  if (big_cleanup || small_cleanup || bad_cleanup || dup_cleanup) {
    for (i = 0; i < ner; ++i) {
      if (!regions_pt[i].maxstable) continue;
      if (bad_cleanup && regions_pt[i].variation >= 1) goto remove_this_region;
      if (big_cleanup && regions_pt[i].area > nel / 2) goto remove_this_region;
      if (small_cleanup && regions_pt[i].area < 25) goto remove_this_region;
      if (dup_cleanup) {
        idx_t parent = regions_pt[i].parent;
        if (parent != i) {
          while (!regions_pt[parent].maxstable) {
            idx_t next = regions_pt[parent].parent;
            if (next == parent) break;
            parent = next;
          }
          int area = regions_pt[i].area;
          int parea = regions_pt[parent].area;
          int change = (parea - area) / (area * 1);
          if (change < 0.5) goto remove_this_region;
        }
      }
      continue;
    remove_this_region:
      regions_pt[i].maxstable = 0;
      --nmer;
    }
  }

  /* Fit ellipses */
  if (nout >= 1) {
    int midx = 1;
    int d, index, j;
    for (i = 0; i < ner; ++i) {
      if (!regions_pt[i].maxstable) continue;
      regions_pt[i].maxstable = midx++;
    }

    for (d = 0; d < (gdl * nmer); d++) sref(ell_pt, d) = 0;

    for (d = 0; d < gdl; ++d) {
      for (int counter_i = 0; counter_i < ndims; counter_i++) sref(subs_pt, counter_i) = 0;

      if (d < ndims) {
        for (index = 0; index < nel; ++index) {
          sref(acc_pt, index) = sref(subs_pt, d);
          adv(dims, ndims, subs_pt);
        }
      } else {
        i = d - ndims;
        j = 0;
        while (i > j) { i -= j + 1; j++; }
        for (index = 0; index < nel; ++index) {
          sref(acc_pt, index) = sref(subs_pt, i) * sref(subs_pt, j);
          adv(dims, ndims, subs_pt);
        }
      }

      for (i = 0; i < njoins; ++i) {
        idx_t index = sref(joins_pt, i);
        idx_t parent = forest_pt[index].parent;
        sref(acc_pt, parent) += sref(acc_pt, index);
      }

      for (i = 0; i < ner; ++i) {
        idx_t region = regions_pt[i].maxstable;
        if (region-- == 0) continue;
        sref(ell_pt, d + gdl * region) = sref(acc_pt, regions_pt[i].index);
      }
    }
  }

  /* Save back */
  {
    int j = 0;
    out = (I2D *)spt; 
    out->height = 1;
    out->width = nmer;

    for (i = 0; i < ner; ++i) {
      if (regions_pt[i].maxstable) {
        asubsref(out, j++) = regions_pt[i].index + 1;
      }
    }
  }

  return out;
}