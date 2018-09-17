//go:generate gorunpkg github.com/99designs/gqlgen

package custom_scalars

import (
	"context"
)

func NewResolver() ResolverRoot {
	return &Resolver{}
}

type Resolver struct{}

func (r *Resolver) Query() QueryResolver {
	return &queryResolver{r}
}

type queryResolver struct{ *Resolver }

func (r *queryResolver) Sample(ctx context.Context) (*Sample, error) {
	panic("not implemented")
}
