# Source the user's primary .zshrc
if [[ -f $HOME/.zshrc ]]; then
    # Unset ZDOTDIR temporarily so it finds the home .zshrc
    OLD_ZDOTDIR=$ZDOTDIR
    unset ZDOTDIR
    source $HOME/.zshrc
    export ZDOTDIR=$OLD_ZDOTDIR
    unset OLD_ZDOTDIR
fi

# Source Pforge completion if available
if [[ -n "$ZDOTDIR" && -f "$ZDOTDIR/../../misc/pforge-completion.zsh" ]]; then
    source "$ZDOTDIR/../../misc/pforge-completion.zsh"
fi
