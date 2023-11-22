#pragma once

#include <string>
#include <cstring>

namespace free_join {
    struct TableBinding {
        std::string binding;
        std::string table;

        TableBinding() = default;

        TableBinding(const TableBinding &other) {
            binding = other.binding;
            table = other.table;
        }
    };

    struct BindingAttribute {
        uint16_t fj_attribute_idx = -1;
        uint16_t table_attribute_idx = -1;
        std::string binding;
        std::string attribute;


        inline bool operator==(const BindingAttribute &rhs) const {
            // cheaper equality comparison
            return binding == rhs.binding && attribute == rhs.attribute;
        }

        inline bool operator<(const BindingAttribute &rhs) const {
            return (binding < rhs.binding) || ((binding == rhs.binding) && (attribute < rhs.attribute));
        }

        BindingAttribute() = default;

        BindingAttribute(const std::string &binding, const std::string &attribute) : binding(binding),
                                                                                     attribute(attribute) {};

        BindingAttribute(const BindingAttribute &other) {
            fj_attribute_idx = other.fj_attribute_idx;
            table_attribute_idx = other.table_attribute_idx;
            binding = other.binding;
            attribute = other.attribute;
        }
    };

    struct BindingAttributeCompare {
        inline bool operator()(const BindingAttribute &lhs, const BindingAttribute &other) const {
            return strcmp(lhs.binding.c_str(), other.binding.c_str()) < 0 ||
                   (strcmp(lhs.binding.c_str(), other.binding.c_str()) == 0 &&
                    strcmp(lhs.attribute.c_str(), other.attribute.c_str()) < 0);
        }
    };

    struct EqualityCondition {
        BindingAttribute left;
        BindingAttribute right;
    };

    enum OperatorType {
        LOGICAL_OPERATOR = 0,
        LOGICAL_GET,
        LOGICAL_BINARY_JOIN,
        LOGICAL_NARY_JOIN,
        LOGICAL_PROJECTION,
        LOGICAL_FILTER,
    };

    class LogicalOperator {
    private:
        OperatorType type = LOGICAL_OPERATOR;
    public:
        LogicalOperator() = default;

        virtual ~LogicalOperator() = default;

        [[nodiscard]] virtual OperatorType get_type() const { return type; }

    };
}
