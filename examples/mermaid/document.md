# Release process

Every change goes through the same steps. The diagrams below are written in
Mermaid inside this Markdown text, and drawn by Raven itself.

```mermaid
flowchart LR
    start([Pull request]) --> ci{CI green?}
    ci -->|no| fix[Fix the build]
    fix --> ci
    ci -->|yes| review{Approved?}
    subgraph checks [Checks]
        direction TB
        build[[Build]] --> tests[(Test results)]
        tests --> lint[/Lint/]
    end
    start -.-> build
    review -->|changes asked| fix
    review ==>|yes| merge((Merge))
    merge --> release{{Tag a release}}
    classDef done fill:#1e9e6a,stroke:#15784f,color:#ffffff
    class merge done
```

## Signing in

```mermaid
sequenceDiagram
    autonumber
    actor U as User
    participant A as App
    participant S as Server
    U->>A: Opens the app
    A->>+S: POST /session
    alt valid password
        S-->>A: token
        Note right of S: kept 30 days
    else wrong password
        S-->>-A: 401
        A->>U: Try again
    end
    loop every 5 minutes
        A->>S: refresh
    end
    Note over U,A: the user stays signed in
```

## The life of a ticket

```mermaid
stateDiagram-v2
    [*] --> Open
    Open --> Working : assigned
    state Working {
        [*] --> Coding
        Coding --> Review : pushed
        Review --> Coding : changes
        Review --> [*] : approved
    }
    Working --> Done
    Open --> Closed : duplicate
    Done --> [*]
    Closed --> [*]
    note right of Closed : never reopened
```

## Where the time goes

```mermaid
pie showData
    title Hours in a sprint
    "Features" : 42
    "Bugs" : 21
    "Reviews" : 12
    "Meetings" : 9
```
