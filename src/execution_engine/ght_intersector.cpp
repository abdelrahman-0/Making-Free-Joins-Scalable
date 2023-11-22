#include "execution_engine/chunked_list_builder.h"
#include "execution_engine/ght_intersector.h"
#include <immintrin.h>
#include "utils/custom_hash.h"

using namespace free_join;

inline void cross_product(std::unordered_map<int16_t, TupleVec> &data,
                          std::map<int16_t, void *>::iterator current_it,
                          const std::map<int16_t, void *>::iterator end_it,
                          TupleVec *output,
                          Tuple &output_tuple,
                          size_t tuple_attr_idx,
                          std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map) {
    // pass iterator recursively
    if (current_it == end_it) {
        output->push_back(output_tuple);
    } else {
        auto current_src_idx = current_it->first;
        for (auto idx: *reinterpret_cast<std::vector<size_t> *>(uintptr_t(current_it->second) & (-1ul >> 1))) {
            auto tmp_tuple_idx = tuple_attr_idx;
            for (const auto &phys_idx: physical_idx_map[current_src_idx]) {
                output_tuple[tmp_tuple_idx++] = data[current_src_idx][idx][phys_idx];
            }
            cross_product(data,
                          std::next(current_it),
                          end_it,
                          output,
                          output_tuple,
                          tmp_tuple_idx,
                          physical_idx_map);
        }
    }

}


inline void cross_product_cl(std::map<int16_t, void *>::iterator current_it,
                             const std::map<int16_t, void *>::iterator end_it,
                             ChunkedList<Tuple, max_chunk_size_tuple> &output,
                             Tuple &output_tuple,
                             size_t tuple_attr_idx,
                             std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map) {
    // pass iterator recursively
    if (current_it == end_it) {
        output.insert(output_tuple);
    } else {
        auto current_src_idx = current_it->first;
        for (auto chunk: *reinterpret_cast<ChunkedList<Tuple, max_chunk_size_tuple> *>(uintptr_t(current_it->second) &
                                                                                       (-1ul >> 1))) {
            for (auto tuple_idx = 0u; tuple_idx < chunk.size; ++tuple_idx) {
                auto tmp_tuple_idx = tuple_attr_idx;
                for (const auto &phys_idx: physical_idx_map[current_src_idx]) {
                    output_tuple[tmp_tuple_idx++] = chunk.data[tuple_idx][phys_idx];
                }
                cross_product_cl(std::next(current_it),
                                 end_it,
                                 output,
                                 output_tuple,
                                 tmp_tuple_idx,
                                 physical_idx_map);
            }
        }
    }

}

//inline void intersect_tries(std::unordered_map<int16_t, TupleVec> &data,
//                            const FreeJoinPlan &plan,
//                            std::map<int16_t, void *> &trie_map,
//                            std::vector<int32_t> &tuple,
//                            size_t i,
//                            TupleVec *output,
//                            Tuple &output_tuple,
//                            std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map,
//                            int16_t leftmost_src_idx,
//                            size_t tuple_idx) {
//    if (i == plan.plan.size()) {
//        auto tmp_tuple_idx = 0u;
//        for (const auto &phys_idx: physical_idx_map[leftmost_src_idx]) {
//            output_tuple[tmp_tuple_idx++] = data[leftmost_src_idx][tuple_idx][phys_idx];
//        }
//        // cross product
//        cross_product(data,
//                      trie_map.begin(),
//                      trie_map.end(),
//                      output,
//                      output_tuple,
//                      tmp_tuple_idx,
//                      physical_idx_map);
//    } else {
//        // choose cover
//        const auto &atoms = plan.plan[i].atoms;
//        auto cover_src_idx = atoms[0].src_index;
//        auto min_size = reinterpret_cast<GHT *>(trie_map.find(cover_src_idx)->second)->node.size();
//        for (auto atom_it = atoms.begin() + 1; atom_it != atoms.end(); ++atom_it) {
//            if (atom_it->is_cover) {
//                auto atom_src_index = atom_it->src_index;
//                auto atom_size = reinterpret_cast<GHT *>(trie_map.find(atom_src_index)->second)->node.size();
//                if (atom_size < min_size) {
//                    min_size = atom_size;
//                    cover_src_idx = atom_src_index;
//                }
//            }
//        }
//
//        auto new_trie_map = trie_map; // copy by assignment
//
//        // loop through cover's data
//        auto j = 0u;
//        auto cover_schema = reinterpret_cast<GHT *>(trie_map.find(cover_src_idx)->second)->schema;
//        for (const auto &val: reinterpret_cast<GHT *>(trie_map.find(cover_src_idx)->second)->node) {
//
//            // update current tuple
//            j = 0u;
//            for (const auto &attr: cover_schema) {
//                tuple[attr.fj_attribute_idx] = (val.first)[j++];
//            }
//
//            // loop through other tries
//            for (const auto &atom: atoms) {
//                if (atom.src_index == cover_src_idx) {
//                    continue;
//                }
//                auto probe_tuple = std::vector<int32_t>(atom.attributes.size());
//                j = 0u;
//
//                // prepare probe tuple
//                for (const auto &attr: atom.attributes) {
//                    probe_tuple[j++] = tuple[attr.fj_attribute_idx];
//                }
//
//                const auto &map = reinterpret_cast<GHT *>(trie_map.find(atom.src_index)->second)->node;
//                auto found = map.find(probe_tuple);
//                if (found != map.end()) {
//                    // found
//                    new_trie_map[atom.src_index] = found->second;
//                } else {
//                    goto continue_outer_loop;
//                }
//            }
//            // recursive
//            new_trie_map[cover_src_idx] = val.second;
//            intersect_tries(data,
//                            plan,
//                            new_trie_map,
//                            tuple,
//                            i + 1,
//                            output,
//                            output_tuple,
//                            physical_idx_map,
//                            leftmost_src_idx,
//                            tuple_idx);
//            continue_outer_loop:;
//        }
//    }
//}
//
//
//std::vector<TupleVec *>
//GHTIntersector::intersect(int16_t leftmost_src_idx,
//                          std::unordered_map<int16_t, TupleVec> &data,
//                          const std::vector<size_t> &leftmost_physical_schema,
//                          const std::map<int16_t, void *> &trie_map,
//                          const FreeJoinPlan &plan,
//                          size_t num_out_attributes,
//                          std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map) {
//    // determine maximum free join index
//    uint16_t max_fj_idx = 0;
//    for (const auto &node: plan.plan) {
//        for (const auto &atom: node.atoms) {
//            for (const auto &attr: atom.attributes) {
//                max_fj_idx = std::max(max_fj_idx, attr.fj_attribute_idx);
//            }
//        }
//    }
//
//    tbb::enumerable_thread_specific<TupleVec *> tls;
//
//    auto range = Range(0, data[leftmost_src_idx].size());
//    tbb::parallel_for(range,
//                      [&](const Range &r) {
//                          if (!tls.local()) {
//                              tls.local() = new TupleVec();
//                          }
//                          // first relation has a one-level schema
//                          auto leftmost_schema = (plan.schema_map.find(leftmost_src_idx))->second[0];
//                          // identity hashing
//                          auto tuple = std::vector<int32_t>(max_fj_idx + 1);
//                          auto new_trie_map = trie_map; // copy constructor
//                          auto output_tuple = Tuple(num_out_attributes);
//                          auto i = 0u;
//                          for (size_t tuple_idx = r.begin(); tuple_idx != r.end(); ++tuple_idx) {
//                              i = 0;
//                              for (const auto &attr: leftmost_schema) {
//                                  tuple[attr.fj_attribute_idx] = data[leftmost_src_idx][tuple_idx][leftmost_physical_schema[i++]].value_.integer;
//                              }
//                              for (auto atom = plan.plan[0].atoms.begin() + 1;
//                                   atom != plan.plan[0].atoms.end(); ++atom) {
//                                  auto probe_tuple = std::vector<int32_t>(atom->attributes.size());
//                                  i = 0;
//                                  for (const auto &a: atom->attributes) {
//                                      probe_tuple[i++] = tuple[a.fj_attribute_idx];
//                                  }
//                                  // top level is always a trie
//                                  const auto &map = reinterpret_cast<GHT *>(trie_map.find(
//                                          atom->src_index)->second)->node;
//                                  auto found = map.find(probe_tuple);
//                                  if (found != map.end()) {
//                                      // found
//                                      new_trie_map[atom->src_index] = found->second;
//                                  } else {
//                                      goto continue_outer_loop;
//                                  }
//                              }
//
//                              intersect_tries(data,
//                                              plan,
//                                              new_trie_map,
//                                              tuple,
//                                              1,
//                                              tls.local(),
//                                              output_tuple,
//                                              physical_idx_map,
//                                              leftmost_src_idx,
//                                              tuple_idx);
//                              continue_outer_loop:;
//                          }
//                      });
//    return {tls.begin(), tls.end()};
//}

//////////////// Version 2 ///////////////

inline void intersect_tries(std::unordered_map<int16_t, TupleVec> &data,
                            const FreeJoinPlan &plan,
                            std::map<int16_t, std::vector<void *>> &trie_map,
                            std::vector<int32_t> &tuple,
                            size_t i,
                            TupleVec *output,
                            Tuple &output_tuple,
                            std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map,
                            int16_t leftmost_src_idx,
                            size_t tuple_idx) {
    if (i == plan.plan.size()) {
        auto tmp_tuple_idx = 0u;
        for (const auto &phys_idx: physical_idx_map[leftmost_src_idx]) {
            output_tuple[tmp_tuple_idx++] = data[leftmost_src_idx][tuple_idx][phys_idx];
        }

        // cross product
        std::map<int16_t, void *> new_trie_map;
        for (const auto &[k, v]: trie_map) {
            new_trie_map[k] = v[0];
        }

        cross_product(data,
                      new_trie_map.begin(),
                      new_trie_map.end(),
                      output,
                      output_tuple,
                      tmp_tuple_idx,
                      physical_idx_map);
    } else {
        // choose cover
        const auto &atoms = plan.plan[i].atoms;
        auto cover_src_idx = atoms[0].src_index;
        auto min_size = 0u;

        for (auto ptr: trie_map.find(cover_src_idx)->second) {
            min_size += reinterpret_cast<GHT *>(ptr)->node.size();
        }


        for (auto atom_it = atoms.begin() + 1; atom_it != atoms.end(); ++atom_it) {
            if (atom_it->is_cover) {
                auto atom_src_index = atom_it->src_index;
                auto atom_size = 0u;
                for (auto ptr: trie_map.find(atom_src_index)->second) {
                    atom_size += reinterpret_cast<GHT *>(ptr)->node.size();
                }
                if (atom_size < min_size) {
                    min_size = atom_size;
                    cover_src_idx = atom_src_index;
                }
            }
        }

        auto new_trie_map = trie_map; // copy by assignment

        // loop through cover's data
        auto j = 0u;
        auto &cover_partial_tries = trie_map.find(cover_src_idx)->second;
        auto &cover_schema = reinterpret_cast<GHT *>(cover_partial_tries[0])->schema;

        // loop through cover split tries
        for (const auto &cover_partial_trie: cover_partial_tries) {
            auto &node = reinterpret_cast<GHT *>(cover_partial_trie)->node;
            for (const auto &it: node) {
                // update current tuple
                j = 0u;
                for (const auto &attr: cover_schema) {
                    tuple[attr.fj_attribute_idx] = (it.first)[j++];
                }

                // loop through other tries
                for (const auto &atom: atoms) {
                    if (atom.src_index == cover_src_idx) {
                        continue;
                    }
                    auto probe_tuple = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(atom.attributes.size());
                    j = 0u;

                    // prepare probe tuple
                    for (const auto &attr: atom.attributes) {
                        probe_tuple[j++] = tuple[attr.fj_attribute_idx];
                    }

                    auto &probe_partial_tries = trie_map.find(atom.src_index)->second;
                    bool found_split_trie = false;
                    for (auto split_trie_idx = 0u; split_trie_idx < probe_partial_tries.size(); ++split_trie_idx) {
                        const auto &map = reinterpret_cast<GHT *>(probe_partial_tries[split_trie_idx])->node;
                        auto found = map.find(probe_tuple);
                        if (found != map.end()) {
                            // found
                            new_trie_map[atom.src_index] = {found->second};
                            found_split_trie = true;
                            break;
                        }
                    }
                    if (!found_split_trie) {
                        goto continue_outer_loop;
                    }
                }
                // recursive
                new_trie_map[cover_src_idx] = {it.second};
                intersect_tries(data,
                                plan,
                                new_trie_map,
                                tuple,
                                i + 1,
                                output,
                                output_tuple,
                                physical_idx_map,
                                leftmost_src_idx,
                                tuple_idx);
                continue_outer_loop:;
            }
        }
    }
}

std::vector<TupleVec *>
GHTIntersector::intersect(int16_t leftmost_src_idx,
                          std::unordered_map<int16_t, TupleVec> &data,
                          const std::vector<size_t, MyAllocator> &leftmost_physical_schema,
                          const std::map<int16_t, std::vector<void *>> &trie_map,
                          const FreeJoinPlan &plan,
                          size_t num_out_attributes,
                          std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map) {
    // determine maximum free join index
    uint16_t max_fj_idx = 0;
    for (const auto &node: plan.plan) {
        for (const auto &atom: node.atoms) {
            for (const auto &attr: atom.attributes) {
                max_fj_idx = std::max(max_fj_idx, attr.fj_attribute_idx);
            }
        }
    }

    tbb::enumerable_thread_specific<TupleVec *> tls;

    auto &leftmost_schema = (plan.schema_map.find(leftmost_src_idx))->second[0];
    auto range = Range(0, data[leftmost_src_idx].size());
    tbb::parallel_for(range,
                      [&](const Range &r) {
                          if (!tls.local()) {
                              tls.local() = new TupleVec();
                          }
                          // first relation has a one-level schema
                          // identity hashing
                          auto tuple = std::vector<int32_t>(max_fj_idx + 1);
                          auto new_trie_map = trie_map; // copy constructor
                          auto output_tuple = Tuple(num_out_attributes);
                          size_t i;
                          for (size_t tuple_idx = r.begin(); tuple_idx != r.end(); ++tuple_idx) {
                              i = 0;
                              for (const auto &attr: leftmost_schema) {
                                  tuple[attr.fj_attribute_idx] = data[leftmost_src_idx][tuple_idx][leftmost_physical_schema[i++]].value_.integer;
                              }
                              for (auto atom = plan.plan[0].atoms.begin() + 1;
                                   atom != plan.plan[0].atoms.end(); ++atom) {
                                  auto probe_tuple = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(
                                          atom->attributes.size());
                                  i = 0;
                                  for (const auto &a: atom->attributes) {
                                      probe_tuple[i++] = tuple[a.fj_attribute_idx];
                                  }
                                  // top level is always a trie
                                  auto split_trie = (trie_map.find(atom->src_index)->second);
                                  bool found_split_trie = false;
                                  for (auto split_trie_idx = 0u; split_trie_idx < split_trie.size(); ++split_trie_idx) {
                                      auto *trie_ptr = split_trie[split_trie_idx];
                                      const auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
                                      auto found = map.find(probe_tuple);
                                      if (found != map.end()) {
                                          // found
                                          // new_trie_map[atom->src_index]
                                          new_trie_map[atom->src_index] = {found->second};
                                          found_split_trie = true;
                                          break;
                                      }
                                  }
                                  if (!found_split_trie) {
                                      goto continue_outer_loop;
                                  }
                              }

                              intersect_tries(data,
                                              plan,
                                              new_trie_map,
                                              tuple,
                                              1,
                                              tls.local(),
                                              output_tuple,
                                              physical_idx_map,
                                              leftmost_src_idx,
                                              tuple_idx);
                              continue_outer_loop:;
                          }
                      });
    return {tls.begin(), tls.end()};
}


inline void intersect_tries_direct(std::unordered_map<int16_t, TupleVec> &data,
                                   const FreeJoinPlan &plan,
                                   std::map<int16_t, GHTWrapper> &trie_map,
                                   std::vector<int32_t> &tuple,
                                   size_t i,
                                   TupleVec *output,
                                   Tuple &output_tuple,
                                   std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map,
                                   int16_t leftmost_src_idx,
                                   size_t tuple_idx) {
    if (i == plan.plan.size()) {
        auto tmp_tuple_idx = 0u;
        for (const auto &phys_idx: physical_idx_map[leftmost_src_idx]) {
            output_tuple[tmp_tuple_idx++] = data[leftmost_src_idx][tuple_idx][phys_idx];
        }

        // cross product
        std::map<int16_t, void *> new_trie_map;
        for (const auto &[k, v]: trie_map) {
            new_trie_map[k] = v.ghts[0];
        }

        cross_product(data,
                      new_trie_map.begin(),
                      new_trie_map.end(),
                      output,
                      output_tuple,
                      tmp_tuple_idx,
                      physical_idx_map);
    } else {
        // choose cover
        const auto &atoms = plan.plan[i].atoms;
        auto cover_src_idx = atoms[0].src_index;
        auto min_size = 0u;

        for (auto ptr: trie_map.find(cover_src_idx)->second.ghts) {
            min_size += reinterpret_cast<GHT *>(ptr)->node.size();
        }


        for (auto atom_it = atoms.begin() + 1; atom_it != atoms.end(); ++atom_it) {
            if (atom_it->is_cover) {
                auto atom_src_index = atom_it->src_index;
                auto atom_size = 0u;
                for (auto ptr: trie_map.find(atom_src_index)->second.ghts) {
                    atom_size += reinterpret_cast<GHT *>(ptr)->node.size();
                }
                if (atom_size < min_size) {
                    min_size = atom_size;
                    cover_src_idx = atom_src_index;
                }
            }
        }

        auto new_trie_map = trie_map; // copy by assignment

        // loop through cover's data
        auto j = 0u;
        auto &cover_partial_tries = trie_map.find(cover_src_idx)->second.ghts;
        auto &cover_schema = reinterpret_cast<GHT *>(cover_partial_tries[0])->schema;

        // loop through cover split tries
        for (const auto &cover_partial_trie: cover_partial_tries) {
            auto &node = reinterpret_cast<GHT *>(cover_partial_trie)->node;
            for (const auto &it: node) {
                // update current tuple
                j = 0u;
                for (const auto &attr: cover_schema) {
                    tuple[attr.fj_attribute_idx] = (it.first)[j++];
                }

                // loop through other tries
                for (const auto &atom: atoms) {
                    if (atom.src_index == cover_src_idx) {
                        continue;
                    }
                    auto probe_tuple = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(atom.attributes.size());
                    j = 0u;

                    // prepare probe tuple
                    for (const auto &attr: atom.attributes) {
                        probe_tuple[j++] = tuple[attr.fj_attribute_idx];
                    }

                    auto split_trie = (trie_map.find(atom.src_index)->second);
                    if (split_trie.ghts.size() > 1) {
                        auto hash = CustomHash{}(probe_tuple);
                        auto *trie_ptr = split_trie.ghts[split_trie.mapping[hash
                                >> (64ull - ChunkedListBuilder::power)]];
                        auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
                        auto found = map.find(probe_tuple);
                        if (found == map.end()) {
                            goto continue_outer_loop;
                        }
                        new_trie_map[atom.src_index].ghts = {found->second};
                    } else {
                        auto *trie_ptr = split_trie.ghts[0];
                        auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
                        auto found = map.find(probe_tuple);
                        if (found == map.end()) {
                            goto continue_outer_loop;
                        }
                        new_trie_map[atom.src_index].ghts[0] = found->second;
                    }
                }
                // recursive
                new_trie_map[cover_src_idx].ghts = {it.second};
                intersect_tries_direct(data,
                                       plan,
                                       new_trie_map,
                                       tuple,
                                       i + 1,
                                       output,
                                       output_tuple,
                                       physical_idx_map,
                                       leftmost_src_idx,
                                       tuple_idx);
                continue_outer_loop:;
            }
        }
    }
}


std::vector<TupleVec *>
GHTIntersector::intersect_direct(int16_t leftmost_src_idx,
                                 std::unordered_map<int16_t, TupleVec> &data,
                                 const std::vector<size_t, MyAllocator> &leftmost_physical_schema,
                                 std::map<int16_t, GHTWrapper> &trie_map,
                                 const FreeJoinPlan &plan,
                                 size_t num_out_attributes,
                                 std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map) {
    // determine maximum free join index
    uint16_t max_fj_idx = 0;
    for (const auto &node: plan.plan) {
        for (const auto &atom: node.atoms) {
            for (const auto &attr: atom.attributes) {
                max_fj_idx = std::max(max_fj_idx, attr.fj_attribute_idx);
            }
        }
    }

    tbb::enumerable_thread_specific<TupleVec *> tls;

    auto &leftmost_schema = (plan.schema_map.find(leftmost_src_idx))->second[0];
    auto range = Range(0, data[leftmost_src_idx].size());
    tbb::parallel_for(range,
                      [&](const Range &r) {
                          if (!tls.local()) {
                              tls.local() = new TupleVec();
                          }
                          // first relation has a one-level schema
                          // identity hashing
                          auto tuple = std::vector<int32_t>(max_fj_idx + 1);
                          auto new_trie_map = trie_map; // copy constructor
                          auto output_tuple = Tuple(num_out_attributes);
                          size_t i;
                          for (size_t tuple_idx = r.begin(); tuple_idx != r.end(); ++tuple_idx) {
                              i = 0;
                              for (const auto &attr: leftmost_schema) {
                                  tuple[attr.fj_attribute_idx] = data[leftmost_src_idx][tuple_idx][leftmost_physical_schema[i++]].value_.integer;
                              }
                              for (auto atom = plan.plan[0].atoms.begin() + 1;
                                   atom != plan.plan[0].atoms.end(); ++atom) {
                                  auto probe_tuple = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(
                                          atom->attributes.size());
                                  i = 0;
                                  for (const auto &a: atom->attributes) {
                                      probe_tuple[i++] = tuple[a.fj_attribute_idx];
                                  }
                                  // top level is always a trie
                                  auto split_trie = (trie_map.find(atom->src_index)->second);
                                  auto hash = CustomHash{}(probe_tuple);
                                  auto *trie_ptr = split_trie.ghts[split_trie.mapping[hash
                                          >> (64ull - ChunkedListBuilder::power)]];
                                  auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
                                  auto found = map.find(probe_tuple);
                                  if (found == map.end()) {
                                      goto continue_outer_loop;
                                  }
                                  new_trie_map[atom->src_index].ghts = {found->second};
                              }

                              intersect_tries_direct(data,
                                                     plan,
                                                     new_trie_map,
                                                     tuple,
                                                     1,
                                                     tls.local(),
                                                     output_tuple,
                                                     physical_idx_map,
                                                     leftmost_src_idx,
                                                     tuple_idx);
                              continue_outer_loop:;
                          }
                      });
    return {tls.begin(), tls.end()};
}

// TODO pass leftmost tuple
inline void intersect_tries_direct_cl(const Tuple &leftmost_tuple,
                                      const FreeJoinPlan &plan,
                                      std::map<int16_t, GHTWrapper> &trie_map,
                                      std::vector<int32_t> &tuple,
                                      size_t i,
                                      ChunkedList<Tuple, max_chunk_size_tuple> &output,
                                      Tuple &output_tuple,
                                      std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map,
                                      int16_t leftmost_src_idx) {
    if (i == plan.plan.size()) {
        auto tmp_tuple_idx = 0u;
        for (const auto &phys_idx: physical_idx_map[leftmost_src_idx]) {
            output_tuple[tmp_tuple_idx++] = leftmost_tuple[phys_idx];
        }

        // cross product
        std::map<int16_t, void *> new_trie_map;
        for (const auto &[k, v]: trie_map) {
            new_trie_map[k] = v.ghts[0];
        }

        cross_product_cl(new_trie_map.begin(),
                         new_trie_map.end(),
                         output,
                         output_tuple,
                         tmp_tuple_idx,
                         physical_idx_map);
    } else {
        // choose cover
        const auto &atoms = plan.plan[i].atoms;
        auto cover_src_idx = atoms[0].src_index;
        auto min_size = 0u;

        for (auto ptr: trie_map.find(cover_src_idx)->second.ghts) {
            min_size += reinterpret_cast<GHT *>(ptr)->node.size();
        }


        for (auto atom_it = atoms.begin() + 1; atom_it != atoms.end(); ++atom_it) {
            if (atom_it->is_cover) {
                auto atom_src_index = atom_it->src_index;
                auto atom_size = 0u;
                for (auto ptr: trie_map.find(atom_src_index)->second.ghts) {
                    atom_size += reinterpret_cast<GHT *>(ptr)->node.size();
                }
                if (atom_size < min_size) {
                    min_size = atom_size;
                    cover_src_idx = atom_src_index;
                }
            }
        }

        auto new_trie_map = trie_map; // copy by assignment

        // loop through cover's data
        auto j = 0u;
        auto &cover_partial_tries = trie_map.find(cover_src_idx)->second.ghts;
        auto &cover_schema = reinterpret_cast<GHT *>(cover_partial_tries[0])->schema;

        // loop through cover split tries
        for (const auto &cover_partial_trie: cover_partial_tries) {
            auto &node = reinterpret_cast<GHT *>(cover_partial_trie)->node;
            for (const auto &it: node) {
                // update current tuple
                j = 0u;
                for (const auto &attr: cover_schema) {
                    tuple[attr.fj_attribute_idx] = (it.first)[j++];
                }

                // loop through other tries
                for (const auto &atom: atoms) {
                    if (atom.src_index == cover_src_idx) {
                        continue;
                    }
                    auto probe_tuple = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(atom.attributes.size());
                    j = 0u;

                    // prepare probe tuple
                    for (const auto &attr: atom.attributes) {
                        probe_tuple[j++] = tuple[attr.fj_attribute_idx];
                    }

                    auto split_trie = (trie_map.find(atom.src_index)->second);
                    if (split_trie.ghts.size() > 1) {
                        auto hash = CustomHash{}(probe_tuple);
                        auto *trie_ptr = split_trie.ghts[split_trie.mapping[hash
                                >> (64ull - ChunkedListBuilder::power)]];
                        auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
                        auto found = map.find(probe_tuple);
                        if (found == map.end()) {
                            goto continue_outer_loop;
                        }
                        new_trie_map[atom.src_index].ghts = {found->second};
                    } else {
                        auto *trie_ptr = split_trie.ghts[0];
                        auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
                        auto found = map.find(probe_tuple);
                        if (found == map.end()) {
                            goto continue_outer_loop;
                        }
                        new_trie_map[atom.src_index].ghts[0] = found->second;
                    }
                }
                // recursive
                new_trie_map[cover_src_idx].ghts = {it.second};
                intersect_tries_direct_cl(leftmost_tuple,
                                          plan,
                                          new_trie_map,
                                          tuple,
                                          i + 1,
                                          output,
                                          output_tuple,
                                          physical_idx_map,
                                          leftmost_src_idx);
                continue_outer_loop:;
            }
        }
    }
}


ChunkedList<Tuple, max_chunk_size_tuple>
GHTIntersector::intersect_direct_cl(int16_t leftmost_src_idx,
                                    std::unordered_map<int16_t, ChunkedList<Tuple, max_chunk_size_tuple> *> &data,
                                    const std::vector<size_t, MyAllocator> &leftmost_physical_schema,
                                    std::map<int16_t, GHTWrapper> &trie_map,
                                    const FreeJoinPlan &plan,
                                    size_t num_out_attributes,
                                    std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map) {
    // determine maximum free join index
    uint16_t max_fj_idx = 0;
    for (const auto &node: plan.plan) {
        for (const auto &atom: node.atoms) {
            for (const auto &attr: atom.attributes) {
                max_fj_idx = std::max(max_fj_idx, attr.fj_attribute_idx);
            }
        }
    }

    tbb::enumerable_thread_specific<ChunkedList<Tuple, max_chunk_size_tuple>> tls(
            ChunkedList<Tuple, max_chunk_size_tuple>{});

    auto &leftmost_schema = (plan.schema_map.find(leftmost_src_idx))->second[0];
//    tbb::parallel_for_each(data[leftmost_src_idx]->cbegin(), data[leftmost_src_idx]->cend(),
//                           [&](const Chunk<Tuple, max_chunk_size_tuple> &chunk) {
//                               // first relation has a one-level schema
//                               // identity hashing
//                               auto tuple = std::vector<int32_t>(max_fj_idx + 1);
//                               auto new_trie_map = trie_map; // copy constructor
//                               auto output_tuple = Tuple();
//                               size_t i;
//                               for (auto tuple_idx = 0u; tuple_idx < chunk.size; ++tuple_idx) {
//                                   i = 0;
//                                   for (const auto &attr: leftmost_schema) {
//                                       tuple[attr.fj_attribute_idx] = chunk.data[tuple_idx][leftmost_physical_schema[i++]].value_.integer;
//                                   }
//                                   for (auto atom = plan.plan[0].atoms.begin() + 1;
//                                        atom != plan.plan[0].atoms.end(); ++atom) {
//                                       auto probe_tuple = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(
//                                               atom->attributes.size());
//                                       i = 0;
//                                       for (const auto &a: atom->attributes) {
//                                           probe_tuple[i++] = tuple[a.fj_attribute_idx];
//                                       }
//                                       // top level is always a trie
//                                       auto split_trie = (trie_map.find(atom->src_index)->second);
//                                       auto hash = CustomHash{}(probe_tuple);
//                                       auto *trie_ptr = split_trie.ghts[split_trie.mapping[hash
//                                               >> (64ull - ChunkedListBuilder::power)]];
//                                       auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
//                                       auto found = map.find(probe_tuple);
//                                       if (found == map.end()) {
//                                           goto continue_outer_loop;
//                                       }
//                                       new_trie_map[atom->src_index].ghts = {found->second};
//                                   }
//
//                                   intersect_tries_direct_cl(chunk.data[tuple_idx],
//                                                             plan,
//                                                             new_trie_map,
//                                                             tuple,
//                                                             1,
//                                                             tls.local(),
//                                                             output_tuple,
//                                                             physical_idx_map,
//                                                             leftmost_src_idx);
//                                   continue_outer_loop:;
//                               }
//                           });
    tbb::task_arena arena;
    std::atomic<Chunk<Tuple, max_chunk_size_tuple> *> control(data[leftmost_src_idx]->head);
    arena.execute([&]() {
        // first relation has a one-level schema
        // identity hashing
        Chunk<Tuple, max_chunk_size_tuple> *cur = data[leftmost_src_idx]->head;
        while (true) {
            while (cur && !control.compare_exchange_strong(cur, cur->next));
            if (cur == nullptr) {
                break;
            }
            const Chunk<Tuple, max_chunk_size_tuple> &chunk = *cur;
            auto tuple = std::vector<int32_t>(max_fj_idx + 1);
            auto new_trie_map = trie_map; // copy constructor
            auto output_tuple = Tuple(num_out_attributes);
            size_t i;
            for (auto tuple_idx = 0u; tuple_idx < chunk.size; ++tuple_idx) {
                i = 0;
                for (const auto &attr: leftmost_schema) {
                    tuple[attr.fj_attribute_idx] = chunk.data[tuple_idx][leftmost_physical_schema[i++]].value_.integer;
                }
                for (auto atom = plan.plan[0].atoms.begin() + 1;
                     atom != plan.plan[0].atoms.end(); ++atom) {
                    auto probe_tuple = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(
                            atom->attributes.size());
                    i = 0;
                    for (const auto &a: atom->attributes) {
                        probe_tuple[i++] = tuple[a.fj_attribute_idx];
                    }
                    // top level is always a trie
                    auto split_trie = (trie_map.find(atom->src_index)->second);
                    auto hash = CustomHash{}(probe_tuple);
                    auto *trie_ptr = split_trie.ghts[split_trie.mapping[hash
                            >> (64ull - ChunkedListBuilder::power)]];
                    auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
                    auto found = map.find(probe_tuple);
                    if (found == map.end()) {
                        goto continue_outer_loop;
                    }
                    new_trie_map[atom->src_index].ghts = {found->second};
                }

                intersect_tries_direct_cl(chunk.data[tuple_idx],
                                          plan,
                                          new_trie_map,
                                          tuple,
                                          1,
                                          tls.local(),
                                          output_tuple,
                                          physical_idx_map,
                                          leftmost_src_idx);
                continue_outer_loop:;
            }
        }
    });

    auto cl = *tls.begin();
    for (auto it = tls.begin() + 1; it != tls.end(); ++it) {
        cl.connect(*it);
    }
    return cl;
}


inline void intersect_tries_direct_vectorized(std::unordered_map<int16_t, TupleVec> &data,
                                              const FreeJoinPlan &plan,
                                              std::map<int16_t, GHTWrapper> &trie_map,
                                              std::vector<int32_t> &tuple,
                                              size_t i,
                                              TupleVec *output,
                                              Tuple &output_tuple,
                                              std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map,
                                              int16_t leftmost_src_idx,
                                              size_t tuple_idx) {
    if (i == plan.plan.size()) {
        auto tmp_tuple_idx = 0u;
        for (const auto &phys_idx: physical_idx_map[leftmost_src_idx]) {
            output_tuple[tmp_tuple_idx++] = data[leftmost_src_idx][tuple_idx][phys_idx];
        }

        // cross product
        std::map<int16_t, void *> new_trie_map;
        for (const auto &[k, v]: trie_map) {
            new_trie_map[k] = v.ghts[0];
        }

        cross_product(data,
                      new_trie_map.begin(),
                      new_trie_map.end(),
                      output,
                      output_tuple,
                      tmp_tuple_idx,
                      physical_idx_map);
    } else {
        // choose cover
        const auto &atoms = plan.plan[i].atoms;
        auto cover_src_idx = atoms[0].src_index;
        auto min_size = 0u;

        for (auto ptr: trie_map.find(cover_src_idx)->second.ghts) {
            min_size += reinterpret_cast<GHT *>(ptr)->node.size();
        }


        for (auto atom_it = atoms.begin() + 1; atom_it != atoms.end(); ++atom_it) {
            if (atom_it->is_cover) {
                auto atom_src_index = atom_it->src_index;
                auto atom_size = 0u;
                for (auto ptr: trie_map.find(atom_src_index)->second.ghts) {
                    atom_size += reinterpret_cast<GHT *>(ptr)->node.size();
                }
                if (atom_size < min_size) {
                    min_size = atom_size;
                    cover_src_idx = atom_src_index;
                }
            }
        }

        auto new_trie_map = trie_map; // copy by assignment

        // loop through cover's data
        auto j = 0u;
        auto &cover_partial_tries = trie_map.find(cover_src_idx)->second.ghts;
        auto &cover_schema = reinterpret_cast<GHT *>(cover_partial_tries[0])->schema;

        // loop through cover split tries
        for (const auto &cover_partial_trie: cover_partial_tries) {
            auto &node = reinterpret_cast<GHT *>(cover_partial_trie)->node;
            for (const auto &it: node) {
                // update current tuple
                j = 0u;
                for (const auto &attr: cover_schema) {
                    tuple[attr.fj_attribute_idx] = (it.first)[j++];
                }

                // loop through other tries
                for (const auto &atom: atoms) {
                    if (atom.src_index == cover_src_idx) {
                        continue;
                    }
                    auto probe_tuple = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(atom.attributes.size());
                    j = 0u;

                    // prepare probe tuple
                    for (const auto &attr: atom.attributes) {
                        probe_tuple[j++] = tuple[attr.fj_attribute_idx];
                    }

                    auto split_trie = (trie_map.find(atom.src_index)->second);
                    if (split_trie.ghts.size() > 1) {
                        auto hash = CustomHash{}(probe_tuple);
                        auto *trie_ptr = split_trie.ghts[split_trie.mapping[hash
                                >> (64ull - ChunkedListBuilder::power)]];
                        auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
                        auto found = map.find(probe_tuple);
                        if (found == map.end()) {
                            goto continue_outer_loop;
                        }
                        new_trie_map[atom.src_index].ghts = {found->second};
                    } else {
                        auto *trie_ptr = split_trie.ghts[0];
                        auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
                        auto found = map.find(probe_tuple);
                        if (found == map.end()) {
                            goto continue_outer_loop;
                        }
                        new_trie_map[atom.src_index].ghts[0] = found->second;
                    }
                }
                // recursive
                new_trie_map[cover_src_idx].ghts = {it.second};
                intersect_tries_direct_vectorized(data,
                                                  plan,
                                                  new_trie_map,
                                                  tuple,
                                                  i + 1,
                                                  output,
                                                  output_tuple,
                                                  physical_idx_map,
                                                  leftmost_src_idx,
                                                  tuple_idx);
                continue_outer_loop:;
            }
        }
    }
}

std::vector<TupleVec *>
GHTIntersector::intersect_direct_vectorized(int16_t leftmost_src_idx,
                                            std::unordered_map<int16_t, TupleVec> &data,
                                            const std::vector<size_t, MyAllocator> &leftmost_physical_schema,
                                            std::map<int16_t, GHTWrapper> &trie_map,
                                            const FreeJoinPlan &plan,
                                            size_t num_out_attributes,
                                            std::unordered_map<int16_t, std::vector<size_t >> &physical_idx_map) {
    // determine maximum free join index
    uint16_t max_fj_idx = 0;
    for (const auto &node: plan.plan) {
        for (const auto &atom: node.atoms) {
            for (const auto &attr: atom.attributes) {
                max_fj_idx = std::max(max_fj_idx, attr.fj_attribute_idx);
            }
        }
    }

    tbb::enumerable_thread_specific<TupleVec *> tls;

    auto &leftmost_schema = (plan.schema_map.find(leftmost_src_idx))->second[0];
    auto range = Range(0, data[leftmost_src_idx].size());
    tbb::parallel_for(range,
                      [&](const Range &r) {
                          if (!tls.local()) {
                              tls.local() = new TupleVec();
                          }
                          // first relation has a one-level schema
                          // identity hashing
                          auto output_tuple = Tuple(num_out_attributes);

                          auto block_size = 64ull;
                          std::vector<std::map<int16_t, GHTWrapper>> new_maps(block_size, trie_map);
                          std::vector<std::vector<int32_t>> new_tuples(block_size,
                                                                       std::vector<int32_t>(max_fj_idx + 1));
                          std::vector<bool> used(block_size, false);

                          size_t i;
                          auto block_idx = 0u;
                          for (size_t tuple_idx = r.begin(); tuple_idx != r.end(); ++tuple_idx) {
                              i = 0;
                              for (const auto &attr: leftmost_schema) {
                                  new_tuples[block_idx][attr.fj_attribute_idx] = data[leftmost_src_idx][tuple_idx][leftmost_physical_schema[i++]].value_.integer;
                              }
                              for (auto atom = plan.plan[0].atoms.begin() + 1;
                                   atom != plan.plan[0].atoms.end(); ++atom) {
                                  auto probe_tuple = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(
                                          atom->attributes.size());
                                  i = 0;
                                  for (const auto &a: atom->attributes) {
                                      probe_tuple[i++] = new_tuples[block_idx][a.fj_attribute_idx];
                                  }
                                  // top level is always a trie
                                  auto split_trie = (trie_map.find(atom->src_index)->second);
                                  auto hash = CustomHash{}(probe_tuple);
                                  auto *trie_ptr = split_trie.ghts[split_trie.mapping[hash
                                          >> (64ull - ChunkedListBuilder::power)]];
                                  auto &map = reinterpret_cast<GHT *>(trie_ptr)->node;
                                  auto found = map.find(probe_tuple);
                                  if (found == map.end()) {
                                      used[block_idx] = false;
                                      goto continue_outer_loop;
                                  }
                                  new_maps[block_idx][atom->src_index].ghts = {found->second};
                              }
                              used[block_idx] = true;

                              continue_outer_loop:;
                              if (block_idx == block_size - 1 || tuple_idx == r.end() - 1) {
                                  for (auto j = 0u; j <= block_idx; ++j) {
                                      if (used[j]) {
                                          intersect_tries_direct_vectorized(data,
                                                                            plan,
                                                                            new_maps[j],
                                                                            new_tuples[j],
                                                                            1,
                                                                            tls.local(),
                                                                            output_tuple,
                                                                            physical_idx_map,
                                                                            leftmost_src_idx,
                                                                            tuple_idx);
                                      }
                                  }
                                  block_idx = 0;
                              }
                          }
                      });
    return {tls.begin(), tls.end()};
}