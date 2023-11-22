#include "data_loader/relation.h"

Relation::Relation(std::string &name, std::string &csv_path, json &raw_schema) : name(name), csv_path(csv_path),
                                                                                 raw_schema(raw_schema) {
    filter_schema();
}

void Relation::filter_schema() {
    schema_mask = 0;
    for (auto &col: raw_schema) {
        if (static_cast<std::string>(col["type"]).rfind(integer_data_type, 0) == 0) {
            schema.push_back(col["name"]);
            schema_mask = (schema_mask << 1) | 1;
            num_cols++;
        } else {
            schema_mask <<= 1;
        }
        raw_num_cols++;
    }
}

std::vector<std::string> Relation::get_schema() const{
    return schema;
}

void Relation::load_csv_data() {
//    std::cout << name << std::endl;
//    data.resize(num_cols);
//    uint64_t col_idx = 0, row_idx = 0;
//    std::string s;
//    int32_t val;
//    uint64_t tmp_mask = 1 << (raw_num_cols - 1);
//
//    rapidcsv::Document doc(csv_path, rapidcsv::LabelParams(-1, 0),
//                           rapidcsv::SeparatorParams(',' /* pSeparator */,
//                                                     true /* pTrim */,
//                                                     rapidcsv::sPlatformHasCR /* pHasCR */,
//                                                     true /* pQuotedLinebreaks */),
//                           rapidcsv::ConverterParams(true));
//    for (uint64_t i = 0; i < raw_num_cols; ++i) {
//        if (schema_mask & tmp_mask) {
//            data[col_idx] = doc.GetColumn<int32_t>(i);
//            col_idx++;
//        }
//        tmp_mask >>= 1;
//    }
//    for(auto& e : data[0]){
//        std::cout << e << std::endl;
//    }
//    return;
//    csv2::Reader<csv2::delimiter<','>, csv2::quote_character<'\"'>, csv2::first_row_is_header<false>, csv2::trim_policy::trim_whitespace> csv;
//    csv.mmap(csv_path);
//    std::cout << csv.rows() << " " << csv.cols() << " " << raw_num_cols << std::endl;
//    for (const auto row: csv) {
//        tmp_mask = 1 << (raw_num_cols - 1);
//        col_idx = 0;
//        row_idx++;
//        for (const auto field: row) {
//            if (schema_mask & tmp_mask) {
//                std::string r;
//                field.read_raw_value(r);
//                try {
//                    val = stoi(r);
//                }
//                catch (std::exception &e) {
//                    val = null_value;
//                }
//                data[col_idx++].push_back(val);
//            }
//            tmp_mask >>= 1;
//        }
////        if(col_idx != 2){
////            row.read_raw_value(s);
////            std::cout << s << std::endl;
////        }
//    }
//    for (unsigned i = 0; i < num_cols; ++i) {
//        std::cout << data[i].size() << " ";
//    }
//
//    // skip quotes
}