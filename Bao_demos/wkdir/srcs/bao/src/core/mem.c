static bool pp_alloc_single_page_bank(struct page_pool *pool, uint64_t pattern,
    uint64_t mask, int shift, struct ppages *ppages)
{
    ppages->num_pages = 0;
    spin_lock(&pool->lock);

    bool found = false;

    size_t bit = pool->last;

 
    for (size_t i = 0; i < pool->size; i++) {
    
        if (bit >= pool->size) {
            bit = 0;
        }

       
        if (!bitmap_get(pool->bitmap, bit)) {
            paddr_t candidate_addr = pool->base + (bit * PAGE_SIZE);
            
            
            if (((candidate_addr >> shift) & mask) == pattern) {
                
                ppages->base = candidate_addr;
                ppages->num_pages = 1;
                bitmap_set(pool->bitmap, bit); 
                pool->free--;
                pool->last = bit + 1;
                found = true;
                
                break;
            }
        }
        bit++;
    }

    spin_unlock(&pool->lock);
    return found;
}

struct ppages mem_alloc_single_page_with_bit_pattern(colormap_t colors,
    uint64_t pattern, uint64_t mask, int shift)
{
    struct ppages page = {0}; 
    list_foreach(page_pool_list, struct page_pool, pool) {
        if (pp_alloc_single_page_bank(pool, pattern, mask, shift, &page)) {
            page.colors = colors;
            break; 
        }
    }
    return page;
}

// (vm_0_free_bw_isolate)
//  static bool pp_alloc_single_page_bank(struct page_pool *pool, uint64_t pattern,
//     uint64_t mask, int shift, struct ppages *ppages)
// {
//     ppages->num_pages = 0;
//     spin_lock(&pool->lock);

//     bool found = false;
//     // Inicia a busca a partir da última alocação para otimizar.
//     size_t bit = pool->last;

//     // Varre o pool de memória inteiro no máximo uma vez.
//     for (size_t i = 0; i < pool->size; i++) {
//         // Se chegar ao fim, continua a busca do início do pool.
//         if (bit >= pool->size) {
//             bit = 0;
//         }

//         // Verifica se a página está livre no bitmap.
//         if (!bitmap_get(pool->bitmap, bit)) {
//             paddr_t candidate_addr = pool->base + (bit * PAGE_SIZE);
            
//             // A verificação crucial: o endereço obedece à restrição?
//             if (((candidate_addr >> shift) & mask) == pattern) {
//                 // Sucesso! A página serve.
//                 ppages->base = candidate_addr;
//                 ppages->num_pages = 1;
//                 bitmap_set(pool->bitmap, bit); // Marca como usada.
//                 pool->free--;
                
//                 if (bit == pool->last) {
//                     pool->last = bit + 1;
//                 }
                
//                 found = true;
//                 break; // Para a busca.
//             }
//         }
//         bit++;
//     }

//     spin_unlock(&pool->lock);
//     return found;
// }

// (victim single for test)
//  #define EXCLUDE_BANK_FLAG 0x80000000 // Flag

//  static bool pp_alloc_single_page_bank(struct page_pool *pool, uint64_t pattern,
//      uint64_t mask, int shift, struct ppages *ppages)
//  {
//      ppages->num_pages = 0;
//      spin_lock(&pool->lock);
 
//      bool found = false;
//      size_t bit = pool->last;
 
//     
//      bool exclude = (pattern & EXCLUDE_BANK_FLAG) != 0;
//      uint64_t actual_pattern = pattern & ~EXCLUDE_BANK_FLAG;
 
//      for (size_t i = 0; i < pool->size; i++) {
//          if (bit >= pool->size) {
//              bit = 0;
//          }
 
//          if (!bitmap_get(pool->bitmap, bit)) {
//              paddr_t candidate_addr = pool->base + (bit * PAGE_SIZE);
//              uint64_t current_bank = (candidate_addr >> shift) & mask;
             
//              bool match = (current_bank == actual_pattern);
 
             
//              if ((!exclude && match) || (exclude && !match)) {
//                  ppages->base = candidate_addr;
//                  ppages->num_pages = 1;
//                  bitmap_set(pool->bitmap, bit);
//                  pool->free--;
                 
//                  if (bit == pool->last) {
//                      pool->last = bit + 1;
//                  }
                 
//                  found = true;
//                  break;
//              }
//          }
//          bit++;
//      }
 
//      spin_unlock(&pool->lock);
//      return found;
//  }