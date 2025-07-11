package main

import (
//    "bytes"
    "encoding/json"
//    "fmt"
    "log"
    "os/exec"
//    "sort"
)

//type Node struct {
//    ID    int
//    Name      string
//    Group string
//    Step  int
//}

/*
type Port struct {
    ID        int
    NodeID    int
    Direction int
    Name      string
}
*/

func nodes() []GroupMember {
    pwDumpOut, err := exec.Command("pw-dump").Output()
    if err != nil {
        log.Fatalf("Error running pw-dump: %v", err)
    }

    var objs []map[string]interface{}
    if err := json.Unmarshal(pwDumpOut, &objs); err != nil {
        log.Fatalf("Error json.Unmarshal: %v", err)
    }

    nodes := []GroupMember{}

    for _, obj := range objs {
        t, ok := obj["type"].(string)
        if !ok {
            continue
        }

        info, ok := obj["info"].(map[string]interface{})
        if !ok {
            continue
        }

        props, ok := info["props"].(map[string]interface{})
        if !ok {
            continue
        }

        switch t {
        case "PipeWire:Interface:Node":
            state, _ := info["state"].(string)
            if state != "running" {
                continue
            }

            name, _ := props["node.name"].(string)
            group, ok1 := props["elvira.group"].(string)
            if !ok1 {continue}
            fstep, ok2 := props["elvira.step"].(float64)
            if !ok2 {continue}
            step := int(fstep)
            if err != nil {
                continue
            }

            //idFloat, ok := obj["id"].(float64)
            //if !ok {
            //    continue
            //}
            //id := int(idFloat)

            nodes = append(nodes, GroupMember{
                //ID:    id,
                Name:   name,
                Group: group,
                Step:  step,
            })
         }
    }

    return nodes
}
