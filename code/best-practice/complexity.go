package best_practice

const ComplexityLimit = 200

func NewComplexity() ComplexityRoot {
	complexityRoot := ComplexityRoot{}

	complexityRoot.Query.EventList = func(childComplexity int, first *int) int {
		if first == nil {
			// 指定無しはエラーにしたいけどそれはResolver側に任せる
			return 1 + childComplexity
		}

		return *first * childComplexity
	}

	complexityRoot.Event.CircleList = func(childComplexity int, first *int) int {
		if first == nil {
			// 指定無しはエラーにしたいけどそれはResolver側に任せる
			return 1 + childComplexity
		}

		return *first * childComplexity
	}

	return complexityRoot
}
