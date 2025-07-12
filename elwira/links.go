package main

import (
	"bytes"
	"fmt"
	"log"
	"net/http"
	"os"
	"os/exec"
	"sort"
	"strings"
)

type GroupMember struct {
	Name  string
	Group string
	Step  int
	ToL   string
	ToR   string
}

type Link struct {
	FromNode string
	FromPort string
	ToNode   string
	ToPort   string
}

func loadLinksFromPwLink() []Link {
	cmd := exec.Command("pw-link", "-l")
	out, err := cmd.Output()
	if err != nil {
		log.Fatalf("Kunde inte köra pw-link -l: %v", err)
	}
	return parsePwLinkOutput(string(out))
}

func parsePwLinkOutput(output string) []Link {
	var links []Link
	lines := strings.Split(output, "\n")
	for i := 0; i < len(lines)-1; i++ {
		line := strings.TrimSpace(lines[i])
		if strings.HasPrefix(line, "|->") {
			target := strings.TrimSpace(strings.TrimPrefix(line, "|->"))
			source := strings.TrimSpace(lines[i-1])
			fromParts := strings.SplitN(source, ":", 2)
			toParts := strings.SplitN(target, ":", 2)
			if len(fromParts) != 2 || len(toParts) != 2 {
				continue
			}
			links = append(links, Link{
				FromNode: fromParts[0],
				FromPort: fromParts[1],
				ToNode:   toParts[0],
				ToPort:   toParts[1],
			})
		}
	}
	return links
}

func buildBEHfromG(G []GroupMember, midiSource string) []Link {
	var links []Link
	for _, g := range G {
		from := g.Name + ":out.FL"
		to := g.ToL
		links = append(links, parseLink(from, to))
		from = g.Name + ":out.FR"
		to = g.ToR
		links = append(links, parseLink(from, to))
	}
        links = append(links, parseLink(midiSource, G[0].Name+":in.midi"))





	return links
}

func parseLink(from, to string) Link {
	fromParts := strings.SplitN(from, ":", 2)
	toParts := strings.SplitN(to, ":", 2)
	return Link{
		FromNode: fromParts[0],
		FromPort: fromParts[1],
		ToNode:   toParts[0],
		ToPort:   toParts[1],
	}
}

func linksEqual(a, b Link) bool {
	return a.FromNode == b.FromNode &&
		a.FromPort == b.FromPort &&
		a.ToNode == b.ToNode &&
		a.ToPort == b.ToPort
}

func intersection(a, b []Link) []Link {
	var result []Link
	for _, x := range a {
		for _, y := range b {
			if linksEqual(x, y) {
				result = append(result, x)
				break
			}
		}
	}
	return result
}

func difference(a, b []Link) []Link {
	var result []Link
	for _, x := range a {
		found := false
		for _, y := range b {
			if linksEqual(x, y) {
				found = true
				break
			}
		}
		if !found {
			result = append(result, x)
		}
	}
	return result
}

func addLinks(links []Link) {
	for _, l := range links {
		from := fmt.Sprintf("%s:%s", l.FromNode, l.FromPort)
		to := fmt.Sprintf("%s:%s", l.ToNode, l.ToPort)
		cmd := exec.Command("pw-link", from, to)
		var out bytes.Buffer
		cmd.Stdout = &out
		cmd.Stderr = &out
		err := cmd.Run()
		if err != nil {
			log.Printf("Fel vid länkning %s → %s: %v\n%s", from, to, err, out.String())
		} else {
			log.Printf("Lade till länk: %s → %s", from, to)
		}
	}
}

func removeLinks(links []Link) {
	for _, l := range links {
		from := fmt.Sprintf("%s:%s", l.FromNode, l.FromPort)
		to := fmt.Sprintf("%s:%s", l.ToNode, l.ToPort)
		cmd := exec.Command("pw-link", "-d", from, to)
		var out bytes.Buffer
		cmd.Stdout = &out
		cmd.Stderr = &out
		err := cmd.Run()
		if err != nil {
			log.Printf("Fel vid borttagning %s → %s: %v\n%s", from, to, err, out.String())
		} else {
			log.Printf("Tog bort länk: %s → %s", from, to)
		}
	}
}

func logLinks(title string, links []Link) {
	log.Printf("== %s (%d länkar) ==", title, len(links))
	for _, l := range links {
		log.Printf("  %s:%s -> %s:%s", l.FromNode, l.FromPort, l.ToNode, l.ToPort)
	}
}

func updateToLToR(G []GroupMember, lastToL, lastToR string) []GroupMember {
	n := len(G)
	for i := 0; i < n; i++ {
		if i == n-1 {
			G[i].ToL = lastToL
			G[i].ToR = lastToR
		} else {
			nextName := G[i+1].Name
			G[i].ToL = nextName + ":in.FL"
			G[i].ToR = nextName + ":in.FR"
		}
		log.Printf("Steg %d: %s → ToL=%s, ToR=%s", G[i].Step, G[i].Name, G[i].ToL, G[i].ToR)
	}
	return G
}

func processGroup(groupName string, G []GroupMember, BEF []Link, lastToL, lastToR string, midiSource string) {
	log.Printf("=====================================")
	log.Printf("== Hanterar grupp: %s", groupName)

	sort.SliceStable(G, func(i, j int) bool {
		return G[i].Step < G[j].Step
	})
	log.Println("-- Sorterad grupp enligt step --")
	for _, g := range G {
		log.Printf("  Step %d: %s", g.Step, g.Name)
	}

	G = updateToLToR(G, lastToL, lastToR)

	BEH := buildBEHfromG(G, midiSource)
	logLinks("BEH (förväntade länkar)", BEH)

	nameSet := make(map[string]struct{})
	for _, g := range G {
		nameSet[g.Name] = struct{}{}
	}
	var BEF_G []Link
	for _, l := range BEF {
		if _, ok := nameSet[l.FromNode]; ok {
			BEF_G = append(BEF_G, l)
		} else if _, ok := nameSet[l.ToNode]; ok {
			BEF_G = append(BEF_G, l)
		}
	}
	logLinks("BEF_G (existerande länkar relaterade till grupp)", BEF_G)

	KVA := intersection(BEF_G, BEH)
	TAB := difference(BEF_G, KVA)
	SKA := difference(BEH, KVA)

	logLinks("KVA (behålls)", KVA)
	logLinks("TAB (tas bort)", TAB)
	logLinks("SKA (läggs till)", SKA)

	log.Println("-- Kör pw-link -d på TAB --")
	removeLinks(TAB)
	log.Println("-- Kör pw-link på SKA --")
	addLinks(SKA)
	log.Println("== Färdig med grupp:", groupName)
}

func linksHandler(w http.ResponseWriter, r *http.Request) {
	log.SetFlags(log.LstdFlags | log.Lshortfile)

/*
	N := []GroupMember{
		{Name: "kall", Group: "g1", Step: 0},
		{Name: "prat", Group: "g1", Step: 1},
		{Name: "varm", Group: "g1", Step: 2},

		{Name: "mic1", Group: "g2", Step: 0},
		{Name: "effekt", Group: "g2", Step: 1},
		{Name: "utgång", Group: "g2", Step: 2},
	}
*/
	N := nodes()
	log.Println("== Läser existerande PipeWire-länkar via pw-link -l")
	BEF := loadLinksFromPwLink()
        for _, link := range BEF {
          log.Printf(" link %v", link)
        }
	groups := make(map[string][]GroupMember)
	for _, gm := range N {
		groups[gm.Group] = append(groups[gm.Group], gm)
	}
	log.Printf("== Hittade %d unika grupper", len(groups))

	lastToL := os.Getenv("ELWIRA_AUDIO_SINK_L")
	lastToR := os.Getenv("ELWIRA_AUDIO_SINK_R")
	midiSource := os.Getenv("ELWIRA_MIDI_SOURCE")

	log.Printf("Send output audio to %s",lastToL)
	log.Printf("Send output audio to %s",lastToR)
	log.Printf("Redeive midi from %s",midiSource)

	for groupName, G := range groups {
		processGroup(groupName, G, BEF, lastToL, lastToR, midiSource)
	}
}

func init() {
	http.HandleFunc("/links",linksHandler)
}
