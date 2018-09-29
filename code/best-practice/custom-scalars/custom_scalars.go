package custom_scalars

import (
	"fmt"
	"io"
	"strconv"

	"github.com/99designs/gqlgen/graphql"
)

// MarshalGraphQLInt64Scalar returns int64
// to GraphQL Scalar value marshaller.
func MarshalGraphQLInt64Scalar(v int64) graphql.Marshaler {
	return graphql.WriterFunc(func(w io.Writer) {
		s := fmt.Sprintf(`"%d"`, v)
		w.Write([]byte(s))
	})
}

// UnmarshalGraphQLInt64Scalar returns int64 value
// from GraphQL value.
func UnmarshalGraphQLInt64Scalar(v interface{}) (int64, error) {
	switch v := v.(type) {
	case string:
		return strconv.ParseInt(v, 10, 64)
	case int64:
		return v, nil
	default:
		return 0, fmt.Errorf("%T is not a int64", v)
	}
}
