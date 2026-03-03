void vm_map_mem_region(struct vm* vm, struct vm_mem_region* reg) {
    size_t n = NUM_PAGES(reg->size);
    #if USE_ISOLATION
    bool use_bank_isolation = false;
    uint64_t bank_pattern = 0;
    const unsigned int bank_shift = SHIFT;
    const unsigned int bank_mask = MASK;


    if (vm->id == 0) {
        use_bank_isolation = USE_BANK_VM_0;
        bank_pattern = BANK_VM_0;
        } else if (vm->id == 1) {
        use_bank_isolation = USE_BANK_VM_1;
        bank_pattern = BANK_VM_1;
        } else if (vm->id == 2) {
            use_bank_isolation = USE_BANK_VM_2;
            bank_pattern = BANK_VM_2;
        } else if (vm->id == 3) {
            use_bank_isolation = USE_BANK_VM_3;
            bank_pattern = BANK_VM_3;
        }
        

        if (reg->place_phys && use_bank_isolation) {
            printk("VM%d: Applying address constraint, seeking pattern=0x%i, with NON-CACHEABLE mapping\n",
                vm->id, (unsigned int)bank_pattern);
                
  
                for (size_t i = 0; i < n; i++) {
                    vaddr_t current_va = (vaddr_t)reg->base + (i * PAGE_SIZE);
                    

                    struct ppages single_page = mem_alloc_single_page_with_bit_pattern(
                        reg->colors, bank_pattern, bank_mask, bank_shift);
                        
                        #if DEBUG
                        printk("VM%d, ISO %lx\n", vm->id, single_page.base);
                        #endif
                        
                        if (single_page.num_pages == 0) {
                            ERROR("VM%d: FAILED to allocate page %lu/%lu with address pattern 0x%llx. No suitable free page found.",
                                vm->id, (unsigned long)i + 1, (unsigned long)n, (unsigned long long)bank_pattern);
                                return;
                            }
                            
                            if (mem_alloc_map(&vm->as, SEC_VM_ANY, &single_page, current_va, 1, PTE_VM_FLAGS) != current_va) {
                                // =======================================================================
                                ERROR("VM%d: FAILED to map page at VA 0x%lx to PA 0x%lx",
                                    vm->id, current_va, single_page.base);
                                    return;
                                }
                            }
                            
                            printk("VM%d: Successfully mapped %lu pages obeying pattern 0x%u\n",
                                vm->id, (unsigned long)n, (unsigned int)bank_pattern);
                                
                            } else {
                                // =======================================================================
        #endif
            
        // =======================================================================
        struct ppages* pa_ptr = NULL;
        struct ppages pa_reg;
        if (reg->place_phys) {
            pa_reg = mem_ppages_get(reg->phys, n);
            pa_reg.colors = reg->colors;
            pa_ptr = &pa_reg;
        }
        if (mem_alloc_map(&vm->as, SEC_VM_ANY, pa_ptr, (vaddr_t)reg->base, n, PTE_VM_FLAGS) != (vaddr_t)reg->base) {
            ERROR("VM%d: Failed to map region at VA 0x%lx", vm->id, reg->base);
        }
        #if USE_ISOLATION
    }
    #endif
}

// vm_0_free_bw_isolated()
// void vm_map_mem_region(struct vm* vm, struct vm_mem_region* reg) {
//     size_t n = NUM_PAGES(reg->size);
    
//     #if USE_ISOLATION
//     bool use_bank_isolation = false;
//     uint64_t bank_pattern = 0;
//     const unsigned int bank_shift = SHIFT;
//     const unsigned int bank_mask = MASK;


//     if (vm->id == 0) {
//         use_bank_isolation = false; 
//     } 

//     else if (vm->id == 1) {
//         use_bank_isolation = USE_BANK_VM_1;
//         bank_pattern = BANK_VM_1;
//     } else if (vm->id == 2) {
//         use_bank_isolation = USE_BANK_VM_2;
//         bank_pattern = BANK_VM_2;
//     } else if (vm->id == 3) {
//         use_bank_isolation = USE_BANK_VM_3;
//         bank_pattern = BANK_VM_3;
//     }
        

//     if (reg->place_phys && use_bank_isolation) {
//         printk("VM%d: Applying address constraint, seeking pattern=0x%i, with NON-CACHEABLE mapping\n",
//             vm->id, (unsigned int)bank_pattern);
            

//         for (size_t i = 0; i < n; i++) {
//             vaddr_t current_va = (vaddr_t)reg->base + (i * PAGE_SIZE);
            
//             // Chama a função estrita original.
//             struct ppages single_page = mem_alloc_single_page_with_bit_pattern(
//                 reg->colors, bank_pattern, bank_mask, bank_shift);
                
//             #if DEBUG
//             printk("VM%d, ISOLAMENTO %lx\n", vm->id, single_page.base);
//             #endif
            
//             if (single_page.num_pages == 0) {
//                 ERROR("VM%d: FAILED to allocate page %lu/%lu with address pattern 0x%llx. No suitable free page found.",
//                     vm->id, (unsigned long)i + 1, (unsigned long)n, (unsigned long long)bank_pattern);
//                 return;
//             }
                
//             if (mem_alloc_map(&vm->as, SEC_VM_ANY, &single_page, current_va, 1, PTE_VM_FLAGS) != current_va) {
//                 ERROR("VM%d: FAILED to map page at VA 0x%lx to PA 0x%lx",
//                     vm->id, current_va, single_page.base);
//                 return;
//             }
//         }
        
//         printk("VM%d: Successfully mapped %lu pages obeying pattern 0x%u\n",
//             vm->id, (unsigned long)n, (unsigned int)bank_pattern);
            
//     } else {
//     #endif
//         // =======================================================================
//         // standart BAO 
//         // =======================================================================
//         struct ppages* pa_ptr = NULL;
//         struct ppages pa_reg;
//         if (reg->place_phys) {
//             pa_reg = mem_ppages_get(reg->phys, n);
//             pa_reg.colors = reg->colors;
//             pa_ptr = &pa_reg;
//         }
//         if (mem_alloc_map(&vm->as, SEC_VM_ANY, pa_ptr, (vaddr_t)reg->base, n, PTE_VM_FLAGS) != (vaddr_t)reg->base) {
//             ERROR("VM%d: Failed to map region at VA 0x%lx", vm->id, reg->base);
//         }
//     #if USE_ISOLATION
//     }
//     #endif
// }
// (victim_single)
// void vm_map_mem_region(struct vm* vm, struct vm_mem_region* reg) {
//     size_t n = NUM_PAGES(reg->size);
    
//     #if USE_ISOLATION
//     bool use_bank_isolation = false;
//     uint64_t bank_pattern = 0;
//     const unsigned int bank_shift = SHIFT;
//     const unsigned int bank_mask = MASK;
    
//     #define EXCLUDE_BANK_FLAG 0x80000000

//     // VM 0 is victim
//     
//     if (vm->id == 0) {
//         use_bank_isolation = true;
//         bank_pattern = BANK_VM_0; 
//     } else if (vm->id == 1 || vm->id == 2 || vm->id == 3) {
//         use_bank_isolation = true;
//         bank_pattern = BANK_VM_0 | EXCLUDE_BANK_FLAG;
//     }

//     if (reg->place_phys && use_bank_isolation) {
//         for (size_t i = 0; i < n; i++) {
//             vaddr_t current_va = (vaddr_t)reg->base + (i * PAGE_SIZE);
            
//             
//             struct ppages single_page = mem_alloc_single_page_with_bit_pattern(
//                 reg->colors, bank_pattern, bank_mask, bank_shift);
                
//             if (single_page.num_pages == 0) {
//                 ERROR("VM%d: FAILED to allocate page %lu/%lu", vm->id, i + 1, n);
//                 return;
//             }

//             // uint64_t allocated_bank = (single_page.base >> bank_shift) & bank_mask;
//             // printk("VM%d: Allocated PA 0x%lx in Bank %llu\n", vm->id, single_page.base, allocated_bank);
//             if (mem_alloc_map(&vm->as, SEC_VM_ANY, &single_page, current_va, 1, PTE_VM_FLAGS) != current_va) {
//                 ERROR("VM%d: FAILED to map page at VA 0x%lx", vm->id, current_va);
//                 return;
//             }
//         }
//         return; 
//     }
//     #endif

//     // Standart BAO
//     struct ppages* pa_ptr = NULL;
//     struct ppages pa_reg;
//     if (reg->place_phys) {
//         pa_reg = mem_ppages_get(reg->phys, n);
//         pa_reg.colors = reg->colors;
//         pa_ptr = &pa_reg;
//     }
//     if (mem_alloc_map(&vm->as, SEC_VM_ANY, pa_ptr, (vaddr_t)reg->base, n, PTE_VM_FLAGS) != (vaddr_t)reg->base) {
//         ERROR("VM%d: Failed to map region at VA 0x%lx", vm->id, reg->base);
//     }
// }