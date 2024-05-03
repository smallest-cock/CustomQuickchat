# Settings Guide

## Bindings

You can either create **button combination** bindings (buttons pressed at the same time), or **button sequence** bindings (buttons pressed in a sequence).

>[!IMPORTANT]
>Button sequence bindings must contain exactly 2 buttons!

## Chats

![](./images/chat_binding_example.png)

Customize the chat, the chat mode, and how it will be triggered.

### Word Variations

![](./images/variation_list_example.png)

Create your own word variation lists in the `Word Variations` tab. Choose a name for the list, then add words in the `variations` section. Each word must be separated by a new line (hence the long text box)

When you're done editing, click `Save`

Then use `[[variation list name]]` syntax in your chats to include your word variations.

**For example:** A chat set up as `hello [[friend]]` will evaluate to: `hello homie` or `hello my guy` etc. based on whatever words/phrases are put in the `friend` variation list.

 - Word variation lists must be at least 3 words long (preferrably 4+)

 - Word variations are chosen randomly from the list... so it's always a surprise 😊


## Special Effects

Similar to variations, you can include these keywords in your chat to trigger special functionality. They require the [Lobby Info plugin](https://github.com/smallest-cock/LobbyInfo) to be installed

>[!TIP]
>Don't name your variation lists the same as these

| keyword | description |
|---|:---:|
`[[blast 1v1]]` | blast the last chatter's 1v1 rank and # of games played
`[[blast 2v2]]` | blast the last chatter's 2v2 rank and # of games played
`[[blast 3v3]]` | blast the last chatter's 3v3 rank and # of games played
`[[blast casual]]` | blast the last chatter's # of casual games played
`[[blast all]]` | blast the last chatter's ranks across all playlists
`[[lastChat]]` | returns the last chat sent in the lobby (depends on your Lobby Info settings)
`[[lastChat sarcasm]]` | returns the last chat iN sArCAsM tExT
`[[lastChat uwu]]` | returns the last chat in UωU text ≽^•⩊•^≼ 👉👈 ( ˶ˆ꒳ˆ˵ )

**For example:** A chat set up as `"[[lastChat]]" - toxic nerd` will wrap the last chatter's chat in quotes, and look as if it were coming from a toxic nerd.