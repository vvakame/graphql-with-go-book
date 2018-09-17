//go:generate gorunpkg github.com/99designs/gqlgen

package complexity_sample

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

func (r *queryResolver) EventList(ctx context.Context, first *int) ([]Event, error) {
	panic("not implemented")
}
