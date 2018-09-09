package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"net/url"
	"os"
)

func main() {
	githubToken := os.Getenv("GITHUB_TOKEN")
	query := `
		query ($login: String!) {
			organization(login: $login) {
				name
				email
			}
		}
	`
	b, err := json.Marshal(struct {
		Query    string                 `json:"query"`
		Variable map[string]interface{} `json:"variables"`
	}{
		Query: query,
		Variable: map[string]interface{}{
			"login": "github",
		},
	})
	if err != nil {
		panic(err)
	}

	endpointURL, err := url.Parse("https://api.github.com/graphql")
	if err != nil {
		panic(err)
	}
	buf := bytes.NewBuffer(b)
	resp, err := http.DefaultClient.Do(&http.Request{
		URL:    endpointURL,
		Method: "POST",
		Header: http.Header{
			"Content-Type":  {"application/json"},
			"Authorization": {"bearer " + githubToken},
		},
		Body: ioutil.NopCloser(buf),
	})
	if err != nil {
		panic(err)
	}
	b, err = ioutil.ReadAll(resp.Body)
	if err != nil {
		panic(err)
	}

	fmt.Println(string(b))
}
