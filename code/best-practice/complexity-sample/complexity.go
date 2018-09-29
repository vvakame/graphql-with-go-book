package complexity_sample

const ComplexityLimit = 200

func NewComplexity() ComplexityRoot {

	f := func(childComplexity int, first *int) int {
		if first == nil {
			// 指定無しはエラーにしたいけどそれはResolver側に任せる
			return 1 + childComplexity
		}

		return *first * childComplexity
	}

	complexityRoot := ComplexityRoot{}
	complexityRoot.Query.EventList = f
	complexityRoot.Event.CircleList = f

	return complexityRoot
}
