package Paquette::Form::LostPassword;
use HTML::FormHandler::Moose;
use HTML::FormHandler::Types ( 'NoSpaces', 'WordChars' );
extends 'HTML::FormHandler';
use Data::Dumper;

with 'HTML::FormHandler::Render::Simple'; # if you want to render the form

has '+item_class' => ( default => 'LostPassword' );

has_field 'email'       => (
    type                => 'Email',
    label               => 'Email',
    required            => 1,
    required_message    => 'You must enter your email address2',
    noupdate            => 1,
    css_class           => 'form_col_a',
);

has_field 'submit'       => ( type => 'Submit', value => 'Save' );

no HTML::FormHandler::Moose;
1;
